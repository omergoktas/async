# async

Basic utility library based on Qt/C++ that runs a function asynchronously in a separate thread which may also be part of a thread pool and returns a QFuture object that will eventually let you handle the results of that function call.

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

## Advanced usage

Please check out following example Qt project for more detailed use cases [asynctest](https://github.com/omergoktas/asynctest)
