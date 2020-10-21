#ifndef INCLUDED_CORO_MCNELLIS_GENERATOR_H
#define INCLUDED_CORO_MCNELLIS_GENERATOR_H

// Original source:
// https://www.youtube.com/watch?v=ZTqHjjm86Bw&t=41m00s (James McNellis, CppCon 2016)

#if __has_include(<coroutine>)
#include <coroutine>
#else
#include <experimental/coroutine>
namespace std {
    using std::experimental::suspend_always;
    using std::experimental::suspend_never;
    using std::experimental::noop_coroutine;
    using std::experimental::coroutine_handle;
}
#endif // __has_include(<coroutine>)

#include <iterator>

template<class T>
struct generator {
    struct promise_type;

    using handle_t = std::coroutine_handle<promise_type>;

    struct promise_type {
        const T *current_;

        generator get_return_object() { return generator(handle_t::from_promise(*this)); }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() { return std::suspend_always{}; }
        void unhandled_exception() {}
        void return_void() {}

        auto yield_value(const T& value) {
            current_ = &value;
            return std::suspend_always{};
        }
    };

    struct iterator {
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        iterator() = default;

        iterator& operator++() {
            coro_.resume();
            if (coro_.done()) {
                coro_ = nullptr;
            }
            return *this;
        }

        const T& operator*() const {
            return *coro_.promise().current_;
        }

        friend bool operator==(const iterator& a, const iterator& b) { return a.coro_ == b.coro_; }
        friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }

    private:
        friend struct generator;
        explicit iterator(handle_t h) : coro_(h) {}

        handle_t coro_;
    };

    iterator begin() {
        if (coro_) {
            coro_.resume();
            if (coro_.done()) {
                return this->end();
            }
        }
        return iterator(coro_);
    }

    iterator end() {
        return iterator{};
    }

    // McNellis's talk doesn't deal with the special member functions.
    generator(generator&& g) : coro_(std::exchange(g.coro_, nullptr)) {}
    generator& operator=(generator) = delete;

    ~generator() {
        if (coro_) {
            coro_.destroy();
        }
    }

private:
    friend struct promise_type;
    explicit generator(handle_t h) : coro_(h) {}

    handle_t coro_;
};

#endif // INCLUDED_CORO_MCNELLIS_GENERATOR_H
