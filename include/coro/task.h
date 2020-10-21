#ifndef INCLUDED_CORO_TASK_H
#define INCLUDED_CORO_TASK_H

// Original source:
// https://github.com/lewissbaker/llvm/blob/9f59dcce/coroutine_examples/manual_lifetime.hpp
// https://github.com/lewissbaker/llvm/blob/9f59dcce/coroutine_examples/task.hpp

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

#include <exception>
#include <memory>

#ifndef INCLUDED_CORO_MANUAL_LIFETIME_H
#define INCLUDED_CORO_MANUAL_LIFETIME_H

template<class T>
struct manual_lifetime {
public:
    manual_lifetime() noexcept {}
    ~manual_lifetime() noexcept {}

    template<class... Args>
    void construct(Args&&... args) {
        ::new (static_cast<void*>(std::addressof(value))) T(static_cast<Args&&>(args)...);
    }

    void destruct() {
        value.~T();
    }

    T& get() & { return value; }
    const T& get() const & { return value; }
    T&& get() && { return (T&&)value; }
    const T&& get() const && { return (const T&&)value; }

private:
  union { T value; };
};

template<class T>
struct manual_lifetime<T&> {
    manual_lifetime() noexcept : ptr(nullptr) {}

    void construct(T& value) noexcept {
        ptr = std::addressof(value);
    }
    void destruct() noexcept {
        ptr = nullptr;
    }

    T& get() const noexcept { return *ptr; }

private:
    T* ptr;
};

template<class T>
struct manual_lifetime<T&&> {
    manual_lifetime() noexcept : ptr(nullptr) {}

    void construct(T&& value) noexcept {
        ptr = std::addressof(value);
    }
    void destruct() noexcept {
        ptr = nullptr;
    }

    T&& get() const noexcept { return *ptr; }

private:
    T* ptr;
};

template<>
struct manual_lifetime<void> {
    void construct() noexcept {}
    void destruct() noexcept {}
    void get() const noexcept {}
};

#endif // INCLUDED_CORO_MANUAL_LIFETIME_H

template<class T>
class task;

template<class T>
class task_promise {
public:
    task_promise() noexcept {}

    ~task_promise() {
        clear();
    }

    task<T> get_return_object() noexcept;

    std::suspend_always initial_suspend() {
        return {};
    }

    auto final_suspend() noexcept {
        struct awaiter {
            bool await_ready() noexcept { return false; }
            auto await_suspend(std::coroutine_handle<task_promise> h) noexcept {
                return h.promise().continuation_;
            }
            void await_resume() noexcept {}
        };
        return awaiter{};
    }

    template<
        class U,
        std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
    void return_value(U&& value) {
        clear();
        value_.construct((U&&)value);
        state_ = state_t::value;
    }

    void unhandled_exception() noexcept {
        clear();
        error_.construct(std::current_exception());
        state_ = state_t::error;
    }

    T get() {
        if (state_ == state_t::error) {
            std::rethrow_exception(std::move(error_).get());
        }
        return std::move(value_).get();
    }

private:
    friend class task<T>;

    void clear() noexcept {
        switch (std::exchange(state_, state_t::empty)) {
        case state_t::empty: break;
        case state_t::error: error_.destruct(); break;
        case state_t::value: value_.destruct(); break;
        }
    }

    std::coroutine_handle<void> continuation_;
    enum class state_t { empty, value, error };
    state_t state_ = state_t::empty;
    union {
        manual_lifetime<T> value_;
        manual_lifetime<std::exception_ptr> error_;
    };
};

template<>
class task_promise<void> {
public:
    task_promise() noexcept {}

    ~task_promise() {
        clear();
    }

    task<void> get_return_object() noexcept;

    std::suspend_always initial_suspend() {
        return {};
    }

    auto final_suspend() {
        struct awaiter {
            bool await_ready() { return false; }
            auto await_suspend(std::coroutine_handle<task_promise> h) {
                return h.promise().continuation_;
            }
            void await_resume() {}
        };
        return awaiter{};
    }

    void return_void() {
        clear();
        value_.construct();
        state_ = state_t::value;
    }

    void unhandled_exception() noexcept {
        clear();
        error_.construct(std::current_exception());
        state_ = state_t::error;
    }

    void get() {
        if (state_ == state_t::error) {
            std::rethrow_exception(std::move(error_).get());
        }
    }

private:
    friend class task<void>;

    void clear() noexcept {
        switch (std::exchange(state_, state_t::empty)) {
            case state_t::empty: break;
            case state_t::error: error_.destruct(); break;
            case state_t::value: value_.destruct(); break;
        }
    }

    enum class state_t { empty, value, error };

    std::coroutine_handle<void> continuation_;
    state_t state_ = state_t::empty;
    union {
        manual_lifetime<void> value_;
        manual_lifetime<std::exception_ptr> error_;
    };
};

template<class T>
class task {
public:
    using promise_type = task_promise<T>;
    using handle_t = std::coroutine_handle<promise_type>;

    explicit task(handle_t h) noexcept
    : coro_(h)
    {}

    task(task&& t) noexcept
    : coro_(std::exchange(t.coro_, {}))
    {}

    ~task() {
        if (coro_) {
            coro_.destroy();
        }
    }

    auto operator co_await() && noexcept {
        struct awaiter {
        public:
            explicit awaiter(handle_t coro) : coro_(coro) {}
            bool await_ready() noexcept {
                return false;
            }
            auto await_suspend(std::coroutine_handle<void> h) noexcept {
                coro_.promise().continuation_ = h;
                return coro_;
            }
            T await_resume() {
                return coro_.promise().get();
            }
        private:
            handle_t coro_;
        };
        return awaiter(coro_);
    }

private:
    handle_t coro_;
};

template<class T>
task<T> task_promise<T>::get_return_object() noexcept
{
    return task<T>(
        std::coroutine_handle<task_promise<T>>::from_promise(*this)
    );
}

inline task<void> task_promise<void>::get_return_object() noexcept
{
    return task<void>(
        std::coroutine_handle<task_promise<void>>::from_promise(*this)
    );
}

#endif // INCLUDED_CORO_TASK_H
