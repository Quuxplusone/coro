#ifndef INCLUDED_CORO_CO_FUTURE_H
#define INCLUDED_CORO_CO_FUTURE_H

// Original source:
// https://stackoverflow.com/questions/55082952/c20-coroutines-implementing-an-awaitable-future
// https://www.youtube.com/watch?v=ZTqHjjm86Bw&t=35m20s (James McNellis, CppCon 2016)

#include <experimental/coroutine>
#include <functional>
#include <future>
#include <type_traits>
#include <utility>

namespace co_future_detail {

template<class T, class CRTP>
struct return_value_or_void {
    template<class U>
    void return_value(U&& value) {
        static_cast<CRTP*>(this)->promise_.set_value(std::forward<U>(value));
    }
};

template<class CRTP>
struct return_value_or_void<void, CRTP> {
    void return_void() {
        static_cast<CRTP*>(this)->promise_.set_value();
    }
};

template<class T>
struct promise_type : public co_future_detail::return_value_or_void<T, promise_type<T>> {
    private:
        friend class co_future_detail::return_value_or_void<T, promise_type<T>>;
        std::promise<T> promise_;
    public:

        auto get_return_object() { return promise_.get_future(); }
        auto initial_suspend() { return std::experimental::suspend_never{}; }
        auto final_suspend() { return std::experimental::suspend_never{}; }

        void set_exception(std::exception_ptr ex) {
            promise_.set_exception(std::move(ex));
        }

        void unhandled_exception() {}
    };

} // namespace co_future_detail

template<class T>
struct co_future : public std::future<T> {
    using std::future<T>::future;
    using std::future<T>::get;
    using std::future<T>::valid;
    using std::future<T>::wait;
    using std::future<T>::wait_for;

    using promise_type = co_future_detail::promise_type<T>;

    co_future(std::future<T>&& f) noexcept : std::future<T>(std::move(f)) {}

    bool is_ready() const {
        return wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    template<class Work>
    auto then(Work&& w) -> co_future<decltype(w())> {
        return { std::async([fut = std::move(*this), w = std::forward<Work>(w)]() mutable {
            fut.wait();
            return w();
        })};
    }

    template<class Work>
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

#endif // INCLUDED_CORO_CO_FUTURE_H
