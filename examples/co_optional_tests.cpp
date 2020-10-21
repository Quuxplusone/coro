// https://coro.godbolt.org/z/n6xbKb

// Original source:
// http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1288r0.pdf
// https://godbolt.org/z/9dapP6

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/co_optional.h>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <thread>

template<template<class> class Monad, class T>
Monad<T> non_coroutine_pure(T&& x) {
    return Monad<T>(std::forward<T>(x));
}

template<template<class> class Monad, class T>
Monad<T> non_coroutine_empty() {
    return {};
}

template<template<class> class Monad>
Monad<int> bar(double x) {
    return int(x * 2);
}

void test_coroutine_lambda() {
    auto result = []() -> co_optional<int> {
        double x = co_await non_coroutine_pure<co_optional>(1.5);  // A
        int y = co_await non_coroutine_pure<co_optional>(int(x * 2));
        co_return y;
    }();
    assert(result.value_or(42) == 3);
}

template<template<class> class Monad>
Monad<int> doblock() {
    double x = co_await non_coroutine_pure<Monad>(1.5);
    int y = co_await non_coroutine_pure<Monad>(int(x * 2));
    co_return y;
}

void test_coroutine_optional() {
    auto result = doblock<co_optional>().value_or(42);
    assert(result == 3);
}

template<template<class> class Monad>
Monad<int> doblock2() {
    double x = co_await non_coroutine_empty<Monad, double>();
    int y = co_await non_coroutine_pure<Monad>(int(x * 2));
    co_return y;
}

void test_coroutine_empty() {
    auto result = doblock2<co_optional>().value_or(42);
    assert(result == 42);
}

int main() {
    test_coroutine_lambda();
    test_coroutine_optional();
    test_coroutine_empty();
    puts("Success!");
}
