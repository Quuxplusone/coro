#ifndef INCLUDED_CORO_GOR_TASK_H
#define INCLUDED_CORO_GOR_TASK_H

// Original source:
// https://www.youtube.com/watch?v=8C8NnE1Dg4A&t=45m50s (Gor Nishanov, CppCon 2016)

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
#include <variant>

template<class T>
struct task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result_;
        std::coroutine_handle<void> waiter_;

        task get_return_object() { return task(this); }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() {
            struct Awaiter {
                promise_type *me_;
                bool await_ready() { return false; }
                void await_suspend(std::coroutine_handle<void> caller) {
                    me_->waiter_.resume();
                }
                void await_resume() {}
            };
            return Awaiter{this};
        }
        template<class U>
        void return_value(U&& u) {
            result_.template emplace<1>(static_cast<U&&>(u));
        }
        void unhandled_exception() {
            result_.template emplace<2>(std::current_exception());
        }
    };

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<void> caller) {
        coro_.promise().waiter_ = caller;
        coro_.resume();
    }
    T await_resume() {
        if (coro_.promise().result_.index() == 2) {
            std::rethrow_exception(std::get<2>(coro_.promise().result_));
        }
        return std::get<1>(coro_.promise().result_);
    }

    ~task() {
        coro_.destroy();
    }
private:
    using handle_t = std::coroutine_handle<promise_type>;
    task(promise_type *p) : coro_(handle_t::from_promise(*p)) {}
    handle_t coro_;
};

#endif // INCLUDED_CORO_GOR_TASK_H
