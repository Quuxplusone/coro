#ifndef INCLUDED_CORO_CO_FUTURE_H
#define INCLUDED_CORO_CO_FUTURE_H

// Original source:
// https://stackoverflow.com/questions/55082952/c20-coroutines-implementing-an-awaitable-future

#include <experimental/coroutine>
#include <functional>
#include <future>
#include <type_traits>
#include <utility>

template<class T>
struct co_future : public std::future<T> {
    using std::future<T>::future;
    using std::future<T>::get;
    using std::future<T>::valid;
    using std::future<T>::wait;
    using std::future<T>::wait_for;

    co_future(std::future<T> && f) noexcept : std::future<T>{std::move(f)} {}

    bool is_ready() const {
        return wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    template <class Work>
    auto then(Work&& w) -> co_future<decltype(w())> {
        return { std::async([fut = std::move(*this), w = std::forward<Work>(w)]() mutable {
            fut.wait();
            return w();
        })};
    }

    template <class Work>
    auto then(Work&& w) -> co_future<decltype(w(std::move(*this)))> {
        return { std::async([fut = std::move(*this), w = std::forward<Work>(w)]() mutable {
            return w(std::move(fut));
        })};
    }

    void await_suspend(std::experimental::coroutine_handle<void> ch) {
        then([ch, this](auto fut) mutable {
            *this = std::move(fut);
            ch.resume();
        });
    }

    bool await_ready() {
        return is_ready();
    }

    auto await_resume() {
        return get();
    }
};

template <class T, class... Arguments>
struct std::experimental::coroutine_traits<co_future<T>, Arguments...> {
    struct promise_type {
    private:
        std::promise<T> _promise;
    public:

        co_future<T> get_return_object() {
            return _promise.get_future();
        }

        auto initial_suspend() {
            return suspend_never{};
        }

        auto final_suspend() {
            return suspend_never{};
        }

        template <class U>
        void return_value(U&& value) {
            _promise.set_value(std::forward<U>(value));
        }

        void set_exception(std::exception_ptr ex) {
            _promise.set_exception(std::move(ex));
        }

        void unhandled_exception() {}
    };
};

#endif // INCLUDED_CORO_CO_FUTURE_H
