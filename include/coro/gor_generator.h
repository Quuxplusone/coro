#ifndef INCLUDED_CORO_GOR_GENERATOR_H
#define INCLUDED_CORO_GOR_GENERATOR_H

// Original source:
// https://godbolt.org/g/26viuZ (Gor Nishanov)

#include <experimental/coroutine>

template<class T>
struct generator {
    struct promise_type {
        T current_value_;
        auto yield_value(T value) {
            this->current_value_ = value;
            return std::experimental::suspend_always{};
        }
        auto initial_suspend() { return std::experimental::suspend_always{}; }
        auto final_suspend() { return std::experimental::suspend_always{}; }
        generator get_return_object() { return generator{this}; };
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    using handle_t = std::experimental::coroutine_handle<promise_type>;

    struct iterator {
        handle_t coro_;
        bool done_;

        iterator() : done_(true) {}

        explicit iterator(handle_t coro, bool done)
            : coro_(coro), done_(done) {}

        iterator& operator++() {
            coro_.resume();
            done_ = coro_.done();
            return *this;
        }

        bool operator==(const iterator& rhs) const { return done_ == rhs.done_; }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
        const T& operator*() const { return coro_.promise().current_value_; }
        const T *operator->() const { return &*(*this); }
    };

    iterator begin() {
        coro_.resume();
        return iterator(coro_, coro_.done());
    }

    iterator end() { return iterator(); }

    generator(const generator&) = delete;
    generator(generator&& rhs) : coro_(std::exchange(rhs.coro_, nullptr)) {}

    ~generator() {
        if (coro_) {
            coro_.destroy();
        }
    }

private:
    explicit generator(promise_type *p)
        : coro_(handle_t::from_promise(*p)) {}

    handle_t coro_;
};

#endif // INCLUDED_CORO_GOR_GENERATOR_H
