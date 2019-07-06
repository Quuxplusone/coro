#ifndef INCLUDED_CORO_RESUMABLE_THING_H
#define INCLUDED_CORO_RESUMABLE_THING_H

// Original source:
// https://www.youtube.com/watch?v=ZTqHjjm86Bw (James McNellis, CppCon 2016)

#include <experimental/coroutine>
#include <utility>

struct resumable_thing {
    struct promise_type;

    using handle_t = std::experimental::coroutine_handle<promise_type>;

    struct promise_type {
        auto get_return_object() {
            return resumable_thing(handle_t::from_promise(*this));
        }
        auto initial_suspend() { return std::experimental::suspend_never{}; }
        auto final_suspend() { return std::experimental::suspend_never{}; }
        void return_void() {}
        void unhandled_exception() {}
    };

    handle_t coro_ = nullptr;

    resumable_thing() = default;
    explicit resumable_thing(handle_t h) : coro_(h) {}
    resumable_thing(const resumable_thing&) = delete;
    resumable_thing(resumable_thing&& rhs) : coro_(std::exchange(rhs.coro_, nullptr)) {}
    resumable_thing& operator=(resumable_thing rhs) { std::swap(coro_, rhs.coro_); return *this; }
    ~resumable_thing() { if (coro_) coro_.destroy(); }

    void resume() {
        coro_.resume();
    }
};

#endif // INCLUDED_CORO_RESUMABLE_THING_H
