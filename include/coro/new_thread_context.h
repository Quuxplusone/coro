#ifndef INCLUDED_CORO_NEW_THREAD_CONTEXT_H
#define INCLUDED_CORO_NEW_THREAD_CONTEXT_H

// Original source:
// https://godbolt.org/z/VIlXUb (Lewis Baker)

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

#include <condition_variable>
#include <mutex>
#include <thread>

// new_thread_context models ExecutionContext.
// It has a .get_executor() method whose result models Executor.

class new_thread_context {
public:
    new_thread_context() = default;

    ~new_thread_context() {
        std::unique_lock<std::mutex> lk(mut_);
        while (activeThreadCount_ != 0) {
            cv_.wait(lk);
        }
    }

private:
    class schedule_awaitable {
    public:
        explicit schedule_awaitable(new_thread_context *context) : context_(context) {}

        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<void> h) {
            if (true) {
                std::lock_guard<std::mutex> lock(context_->mut_);
                ++context_->activeThreadCount_;
            }

            try {
                std::thread t = std::thread([this, h]() mutable {
                    h.resume();
                    std::unique_lock<std::mutex> lock(context_->mut_);
                    --context_->activeThreadCount_;
                    std::notify_all_at_thread_exit(context_->cv_, std::move(lock));
                });
                t.detach();
            } catch (...) {
                std::unique_lock<std::mutex> lock(context_->mut_);
                --context_->activeThreadCount_;
                throw;
            }
        }

        void await_resume() {}

    private:
        new_thread_context *context_;
    };

public:

    struct executor {
    public:
        explicit executor(new_thread_context *context) noexcept :
            context_(context)
        {}

        auto schedule() noexcept {
            return schedule_awaitable(context_);
        }

    private:
        new_thread_context *context_;
    };

    executor get_executor() { return executor(this); }

private:
    std::mutex mut_;
    std::condition_variable cv_;
    size_t activeThreadCount_ = 0;
};

#endif // INCLUDED_CORO_NEW_THREAD_CONTEXT_H
