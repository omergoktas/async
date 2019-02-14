/****************************************************************************
**
** Copyright (C) 2019 Ömer Göktaş
** Contact: omergoktas.com
**
** This file is part of the Async library.
**
** The Async is free software: you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License
** version 3 as published by the Free Software Foundation.
**
** The Async is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with the Async. If not, see
** <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ASYNC_H
#define ASYNC_H

#include "optional.h"

#include <QCoreApplication>
#include <QFuture>
#include <QThreadPool>


#include <iostream>

namespace Async {

using StackSize = Optional<uint>;

namespace Internal {

/*!
    void (QFutureInterface*)         | Plain call  -  No report
    void (no QFutureInterface*)      | Plain call  -  No need to report
    non-void (QFutureInterface*)     | Plain call  -  No report
    non-void (no QFutureInterface*)  | Report call -  Report the result
*/

// Plain call
template <typename ReturnType, typename Function, typename... Args>
void call(std::true_type, QFutureInterface<ReturnType>*, Function&& function, Args&&... args)
{
    std::forward<Function>(function)(std::forward<Args>(args)...);
}

// Report call
template <typename ReturnType, typename Function, typename... Args>
void call(std::false_type, QFutureInterface<ReturnType>* futureInterface, Function&& function, Args&&... args)
{
    futureInterface->reportResult(std::forward<Function>(function)(std::forward<Args>(args)...));
}

// Call with QFutureInterface*
template <typename ReturnType, typename Function, typename... Args>
void runHelperImpl(std::true_type, QFutureInterface<ReturnType>* futureInterface, Function&& function, Args&&... args)
{
    call(std::true_type(), futureInterface,
         std::forward<Function>(function), std::forward<Args>(args)...);
}

// Call without QFutureInterface*
template <typename ReturnType, typename Function, typename... Args>
void runHelperImpl(std::false_type, QFutureInterface<ReturnType>* futureInterface, Function&& function, Args&&... args)
{
    call(std::is_void<std::result_of_t<Function(Args...)>>(), futureInterface,
         std::forward<Function>(function), std::forward<Args>(args)...);
}

// Başarısız çıkarsama bir hata değildir

template <typename...> using void_t = void;

template <typename T, typename = void>
struct is_function_call_viable : std::false_type {};

template <typename T>
struct is_function_call_viable<T, void_t<std::result_of_t<T>>> : std::true_type {};

template <class T> std::decay_t<T> decayCopy(T&& v)
{ return std::forward<T>(v); }

template <typename T, typename Function, typename... Args>
class Runnable : public QRunnable
{
    using ReturnType = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using FunctionStorage = std::tuple<std::decay_t<Function>, std::decay_t<Args>...>;

public:
    Runnable(Function&& function, Args&&... args) : m_priority(QThread::InheritPriority)
      , m_functionStorage(decayCopy(std::forward<Function>(function)), decayCopy(std::forward<Args>(args))...)
    {
        m_futureInterface.setRunnable(this);
        m_futureInterface.reportStarted();
    }

    ~Runnable() override
    { m_futureInterface.reportFinished(); }

    QFuture<ReturnType> future()
    { return m_futureInterface.future(); }

    void setThreadPool(QThreadPool* threadPool)
    { m_futureInterface.setThreadPool(threadPool); }

    void setThreadPriority(QThread::Priority priority)
    { m_priority = priority; }

    void run() override
    {
        if (m_priority != QThread::InheritPriority) {
            if (QThread* thread = QThread::currentThread()) {
                if (thread != qApp->thread())
                    thread->setPriority(m_priority);
            }
        }
        if (m_futureInterface.isCanceled()) {
            m_futureInterface.reportFinished();
            return;
        }

        runHelper(std::make_index_sequence<std::tuple_size<FunctionStorage>::value>());
    }

    template <std::size_t... index>
    void runHelper(std::index_sequence<index...>)
    {
        // invalidates data, which is moved into the call
        runHelperImpl(T(), &m_futureInterface, std::move(std::get<index>(m_functionStorage))...);
        if (m_futureInterface.isPaused())
            m_futureInterface.waitForResume();
        m_futureInterface.reportFinished();
    }

private:
    QThread::Priority m_priority;
    FunctionStorage m_functionStorage;
    QFutureInterface<ReturnType> m_futureInterface;
};

class ASYNC_EXPORT RunnableThread : public QThread
{
public:
    explicit RunnableThread(QRunnable* runnable, QObject* parent = nullptr);

protected:
    void run() override;

private:
    QRunnable* m_runnable;
};

template <typename Runnble>
auto run(Runnble runnable, QThreadPool* pool, QThread::Priority priority, StackSize stackSize)
{
    Q_ASSERT(!(pool && stackSize)); // stack size cannot be changed once a thread is started
    runnable->setThreadPriority(priority);
    if (pool) {
        runnable->setThreadPool(pool);
        pool->start(runnable);
    } else {
        auto thread = new RunnableThread(runnable);
        if (stackSize)
        thread->setStackSize(stackSize.value());
        thread->moveToThread(qApp->thread()); // make sure thread gets deleteLater on main thread
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start(priority);
    }
    return runnable->future();
}

template <typename Function, typename... Args,
          typename = std::enable_if_t<!is_function_call_viable<std::decay_t<Function>(QFutureInterfaceBase*, std::decay_t<Args>...)>::value>
          >
auto run(QThreadPool* pool, QThread::Priority priority, StackSize stackSize,
         Function&& function, Args&&... args)
{
    return run(new Runnable<std::false_type, Function, Args...>(
                   std::forward<Function>(function),
                   std::forward<Args>(args)...), pool, priority, stackSize);
}

template <typename Function, typename... Args,
          typename std::enable_if_t<is_function_call_viable<std::decay_t<Function>(QFutureInterfaceBase*, std::decay_t<Args>...)>::value, int> = 0
          >
auto run(QThreadPool* pool, QThread::Priority priority, StackSize stackSize,
         Function&& function, Args&&... args)
{
    return run(new Runnable<std::true_type, Function, QFutureInterfaceBase*, Args...>(
                   std::forward<Function>(function),
                   new QFutureInterfaceBase,
                   std::forward<Args>(args)...), pool, priority, stackSize);
}
} // Internal

template<typename Function, typename... Args>
auto run(StackSize stackSize, QThread::Priority priority, Function&& function, Args&&... args)
{
    return Internal::run(nullptr, priority, stackSize,
                         std::forward<Function>(function),
                         std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto run(QThreadPool* pool, QThread::Priority priority, Function&& function, Args&&... args)
{
    return Internal::run(pool, priority, StackSize(),
                         std::forward<Function>(function),
                         std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto run(QThread::Priority priority, Function&& function, Args&&... args)
{
    return Internal::run(nullptr, priority, StackSize(),
                         std::forward<Function>(function),
                         std::forward<Args>(args)...);
}

template <typename Function, typename... Args,
          typename = std::enable_if_t<
              !std::is_same<std::decay_t<Function>,
                            QThread::Priority>::value>
          >
auto run(QThreadPool* pool, Function&& function, Args&&... args)
{
    return Internal::run(pool, QThread::InheritPriority, StackSize(),
                         std::forward<Function>(function),
                         std::forward<Args>(args)...);
}

template<typename Function, typename... Args>
auto run(StackSize stackSize, Function&& function, Args&&... args)
{
    return Internal::run(nullptr, QThread::InheritPriority, stackSize,
                         std::forward<Function>(function),
                         std::forward<Args>(args)...);
}

template <typename Function, typename... Args,
          typename = std::enable_if_t<
              !std::is_same<std::decay_t<Function>, QThreadPool>::value &&
              !std::is_same<std::decay_t<Function>, QThread::Priority>::value>>
auto run(Function&& function, Args&&... args)
{
    return Internal::run(nullptr, QThread::InheritPriority, StackSize(),
                         std::forward<Function>(function),
                         std::forward<Args>(args)...);
}
} // Async

#endif // ASYNC_H
