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

namespace Async {

using StackSize = Optional<uint>;

namespace Internal {

template <typename...> using void_t = void;

template <typename T, typename = void>
struct is_overload_viable : std::false_type {};

template <typename T>
struct is_overload_viable<T, void_t<std::result_of_t<T>>> : std::true_type {};

template <typename F, typename... A>
static constexpr bool is_future_overload =
        is_overload_viable<std::decay_t<F>(QFutureInterfaceBase*, std::decay_t<A>...)>::value;

/*!
    void (QFutureInterface*)        | Plain call  -  No report
    void (no QFutureInterface*)     | Plain call  -  No need to report
    non-void (QFutureInterface*)    | Plain call  -  No report
    non-void (no QFutureInterface*) | Report call -  Report the result
*/

// Plain call
template <typename ReturnType, typename Function, typename... Args>
void call(std::true_type, QFutureInterface<ReturnType>*, Function&& function, Args&&... args)
{ std::forward<Function>(function)(std::forward<Args>(args)...); }

// Report call
template <typename ReturnType, typename Function, typename... Args>
void call(std::false_type, QFutureInterface<ReturnType>* future, Function&& function, Args&&... args)
{ future->reportResult(std::forward<Function>(function)(std::forward<Args>(args)...)); }

// Call with QFutureInterface*
template <typename ReturnType, typename Function, typename... Args>
void dispatchFutureCall(std::true_type, QFutureInterface<ReturnType>* future, Function&& function, Args&&... args)
{ call(std::true_type(), future, std::forward<Function>(function), future, std::forward<Args>(args)...); }

// Call without QFutureInterface*
template <typename ReturnType, typename Function, typename... Args>
void dispatchFutureCall(std::false_type, QFutureInterface<ReturnType>* future, Function&& function, Args&&... args)
{ call(std::is_void<std::result_of_t<Function(Args...)>>(), future, std::forward<Function>(function), std::forward<Args>(args)...); }

template <typename is_future, typename ReturnType, typename Function, typename... Args>
class Runnable : public QRunnable
{
    using FunctionStorage = std::tuple<std::decay_t<Function>, std::decay_t<Args>...>;

public:
    Runnable(Function&& function, Args&&... args) : m_priority(QThread::InheritPriority)
      , m_functionStorage(std::decay_t<Function>(std::forward<Function>(function)),
                          std::decay_t<Args>(std::forward<Args>(args))...)
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

        dispatchCall(std::make_index_sequence<std::tuple_size<FunctionStorage>::value>());
    }

private:
    template <std::size_t... index>
    void dispatchCall(std::index_sequence<index...>)
    {
        // invalidates data, which is moved into the call
        dispatchFutureCall(is_future(), &m_futureInterface, std::move(std::get<index>(m_functionStorage))...);
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
auto run(Runnble runnable, QThreadPool* pool, QThread::Priority priority, const StackSize& stackSize)
{
    runnable->setThreadPriority(priority);
    if (pool) {
        Q_ASSERT(!stackSize); // Can't change stack size for QThreadPool
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
          typename = std::enable_if_t<!is_future_overload<Function, Args...>>>
auto run(QThreadPool* pool, QThread::Priority priority, const StackSize& stackSize,
         Function&& function, Args&&... args)
{
    using ReturnType = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    return run(new Runnable<std::false_type, ReturnType, Function, Args...>(
                   std::forward<Function>(function), std::forward<Args>(args)...), pool, priority, stackSize);
}

template <typename Function, typename... Args,
          std::enable_if_t<is_future_overload<Function, Args...>, int> = 0>
auto run(QThreadPool* pool, QThread::Priority priority, const StackSize& stackSize,
         Function&& function, Args&&... args)
{
    using ReturnType = std::result_of_t<std::decay_t<Function>(QFutureInterfaceBase*, std::decay_t<Args>...)>;
    return run(new Runnable<std::true_type, ReturnType, Function, Args...>(
                   std::forward<Function>(function), std::forward<Args>(args)...), pool, priority, stackSize);
}
} // Internal

template <typename Function, typename... Args>
auto run(QThreadPool* pool, QThread::Priority priority, Function&& function, Args&&... args)
{
    return Internal::run(pool, priority, StackSize(),
                         std::forward<Function>(function), std::forward<Args>(args)...);
}

template<typename Function, typename... Args>
auto run(const StackSize& stackSize, QThread::Priority priority, Function&& function, Args&&... args)
{
    return Internal::run(nullptr, priority, stackSize,
                         std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto run(QThread::Priority priority, Function&& function, Args&&... args)
{
    return Internal::run(nullptr, priority, StackSize(),
                         std::forward<Function>(function), std::forward<Args>(args)...);
}

template<typename Function, typename... Args>
auto run(const StackSize& stackSize, Function&& function, Args&&... args)
{
    return Internal::run(nullptr, QThread::InheritPriority, stackSize,
                         std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto run(QThreadPool* pool, Function&& function, Args&&... args)
{
    return Internal::run(pool, QThread::InheritPriority, StackSize(),
                         std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto run(Function&& function, Args&&... args)
{
    return Internal::run(nullptr, QThread::InheritPriority, StackSize(),
                         std::forward<Function>(function), std::forward<Args>(args)...);
}
} // Async

#endif // ASYNC_H
