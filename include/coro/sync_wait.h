#ifndef INCLUDED_CORO_SYNC_WAIT_H
#define INCLUDED_CORO_SYNC_WAIT_H

// Original source:
// https://github.com/lewissbaker/llvm/blob/9f59dcce/coroutine_examples/sync_wait.hpp

#include <condition_variable>
#include <exception>
#include <experimental/coroutine>
#include <mutex>

struct sync_wait_task {
    struct promise_type {
        sync_wait_task get_return_object() noexcept {
            return sync_wait_task(
                std::experimental::coroutine_handle<promise_type>::from_promise(*this)
            );
        }

        auto initial_suspend() {
            return std::experimental::suspend_always{};
        }

        auto final_suspend() noexcept {
            struct awaiter {
                bool await_ready() { return false; }
                void await_suspend(std::experimental::coroutine_handle<promise_type> h) {
                    auto& promise = h.promise();
                    std::lock_guard<std::mutex> lock(promise.mut_);
                    promise.done_ = true;
                    promise.cv_.notify_one();
                }
                void await_resume() {}
            };
            return awaiter{};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            error_ = std::current_exception();
        }

        void wait() {
            std::unique_lock<std::mutex> lk(mut_);
            while (!done_) {
                cv_.wait(lk);
            }

            if (error_) {
                std::rethrow_exception(error_);
            }
        }
    private:
        std::mutex mut_;
        std::condition_variable cv_;
        bool done_ = false;
        std::exception_ptr error_;
    };

    using handle_t = std::experimental::coroutine_handle<promise_type>;

    explicit sync_wait_task(handle_t coro) noexcept
    : coro_(coro) {}

    sync_wait_task(sync_wait_task&& t) noexcept
    : coro_(std::exchange(t.coro_, {}))
    {}

    ~sync_wait_task() {
        if (coro_) {
            coro_.destroy();
        }
    }

    void wait() {
        coro_.resume();
        coro_.promise().wait();
    }

private:
    handle_t coro_;
};

template<class Awaitable>
void sync_wait(Awaitable&& t) {
    [&]() -> sync_wait_task {
        co_await std::move(t);
    }().wait();
}

#endif // INCLUDED_CORO_SYNC_WAIT_H
