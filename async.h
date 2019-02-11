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

template <typename ReturnType> class Runnable : public QRunnable
{
    using Function = ReturnType(QFutureInterfaceBase*);
public:
    Runnable(const std::function<Function>& function) : m_priority(QThread::InheritPriority)
      , m_function(function)
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

        const ReturnType& result = m_function(&m_futureInterface);

        if (m_futureInterface.isPaused())
            m_futureInterface.waitForResume();
        if (!m_futureInterface.isCanceled()) {
            m_futureInterface.setProgressValue(m_futureInterface.progressMaximum());
            m_futureInterface.reportResult(result);
        }
        m_futureInterface.reportFinished();
    }

private:
    QThread::Priority m_priority;
    std::function<Function> m_function;
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

template <typename Function, typename... Args>
auto run(QThreadPool* pool, QThread::Priority priority, StackSize stackSize,
         Function&& function, Args&&... args)
{
    Q_ASSERT(!(pool && stackSize)); // stack size cannot be changed once a thread is started
    using ReturnType = decltype(std::forward<Function>(function) (
    static_cast<QFutureInterfaceBase*>(nullptr),
    std::forward<Args>(args)...
    ));
    auto runnable = new Runnable<ReturnType>(std::bind(std::forward<Function>(function),
                                                       std::placeholders::_1,
                                                       std::forward<Args>(args)...));
    runnable->setThreadPriority(priority);
    QFuture<ReturnType> future = runnable->future();
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
    return future;
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
          typename = std::enable_if_t<!std::is_same<std::decay_t<Function>, QThread::Priority>::value>>
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
