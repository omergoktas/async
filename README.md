# Async

> [!IMPORTANT]  
> This repository is no longer maintained; we suggest using QPromise in your new projects.

Basic utility library based on Qt/C++ that runs a function asynchronously in a separate thread which may also be part of a thread pool and returns a QFuture object that will eventually let you handle the results (states) of that function call. It supports pause/resume/cancel functionality for a task running on a thread. It also supports instant progress reporting via QFutureWatcher class. You can use Qt's signal/slot mechanism to catch those progress changes happens on your worker thread from the main thread.

> C++14 needed


## Example calls

```cpp
//! Plain functions
Async::run(sum, 7, 3);                             // void sum(int, int) {}
Async::run(callback, "Thread is running!");        // void callback(const char*) {}

//! Member functions
Math math;                                         // struct Math { void square(int) {} };
Async::run(Hello::hello);                          // struct Hello { static void hello() {} };
Async::run(std::bind(&Math::square, &math, 5));
Async::run(std::bind(&Math::square, &math,
                     std::placeholders::_1), 7);

//! Lambdas
Async::run([] () {});
Async::run([] (int) {}, 54);
Async::run([] (const QString&) {}, "Oops!!!");
Async::run([] (int, char) {}, 25, '@');
Async::run([] (auto, auto) {}, 'c', 3.9);

//! Other callables
Callable c;                                        // struct Callable {
Async::run(c);                                     //     void operator()() {}
Async::run(Callable());                            //     void operator()(const char*) {}
Async::run(c, "Boom!");                            // };
Async::run(Callable(), "Hey!");
```

## Example progress reporting

```cpp
#include <async.h>
#include <QRandomGenerator>
#include <QApplication>
#include <QFutureWatcher>
#include <QPushButton>
#include <QTimer>

// Example task function
int timeConsumingRandomNumberGenerator(QFutureInterfaceBase* futureInterface, int rangeMin, int rangeMax)
{
    auto future = static_cast<QFutureInterface<int>*>(futureInterface);
    future->setProgressRange(0, 100);
    future->setProgressValue(0);

    int value = QRandomGenerator::global()->bounded(rangeMin, rangeMax);
    for (int i = 1; i <= 100; ++i) {
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return value;
        value = QRandomGenerator::global()->bounded(rangeMin, rangeMax);
        future->setProgressValueAndText(i, QString("Random number: %1").arg(value));
        QThread::msleep(50);
    }

    future->reportResult(value);
    return value;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    //! Run the task asynchronously
    QFutureWatcher<int> watcher;
    watcher.setFuture(Async::run(timeConsumingRandomNumberGenerator, 100, 999));

    //! Pause/resume the task
    QPushButton pauseButton;
    pauseButton.setText("Pause/Resume");
    pauseButton.show();
    QObject::connect(&pauseButton, &QPushButton::clicked, [&] {
        watcher.future().togglePaused();
        qWarning("Operation %s", watcher.isPaused() ? "paused" : "running");
    });

    //! Catch state changes on the task by connecting appropriate signals to slots
    QObject::connect(&watcher, &QFutureWatcherBase::progressTextChanged, [&]
    { qWarning("%s", watcher.progressText().toUtf8().data()); });
    QObject::connect(&watcher, &QFutureWatcherBase::progressValueChanged, [&]
    { qWarning("Progress: %d", watcher.progressValue()); });
    QObject::connect(&watcher, &QFutureWatcherBase::resultReadyAt, [&]
    { qWarning("Result ready, result: %d", watcher.result()); });
    QObject::connect(&watcher, &QFutureWatcherBase::canceled, [&]
    { qWarning("Operation canceled!"); });
    QObject::connect(&watcher, &QFutureWatcherBase::finished, [&]
    { qWarning("Operation finished!"); app.quit(); });
    // Note: See QTBUG-12152 for QFutureWatcherBase::paused signal

    //! Cancel the operation after 1 second
    // QTimer::singleShot(1000, &watcher, &QFutureWatcher<int>::cancel);

    return app.exec();
}
```

## Advanced usage

Please check out following example Qt projects for more detailed use cases [asynctest](https://github.com/omergoktas/asynctest) or [zipasync](https://github.com/omergoktas/zipasync)

<p align="center">
  <img src="https://omergoktas.github.io/githubfiles/asynctest/asynctest.gif">
</p>
