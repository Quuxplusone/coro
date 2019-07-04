// https://coro.godbolt.org/z/XE6R8k

// Original source:
// http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1288r0.pdf
// https://godbolt.org/z/9dapP6

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/concepts.h>
#include <experimental/coroutine>
#include <string>
#include <type_traits>

template<typename T>
struct ready_awaiter {
    T value;

    bool await_ready() { return true; }
    void await_suspend(std::experimental::coroutine_handle<void>) {}
    T await_resume() { return static_cast<T&&>(value); }
};

template<typename T>
struct ready_awaitable {
    T value;

    ready_awaiter<T&> operator co_await() & { return { value }; }
    ready_awaiter<const T&> operator co_await() const & { return { value }; }
    ready_awaiter<T&&> operator co_await() && { return { static_cast<T&&>(value) }; }
    ready_awaiter<const T&&> operator co_await() const && { return { static_cast<const T&&>(value) }; }
};

struct simple_task {
    ready_awaiter<std::string> operator co_await() && {
        return { "hello" };
    }
};

static_assert(Awaitable<std::experimental::suspend_always>);
static_assert(Awaitable<std::experimental::suspend_never>);
static_assert(Awaiter<std::experimental::suspend_always>);
static_assert(Awaiter<std::experimental::suspend_never>);

static_assert(Awaitable<ready_awaiter<int>>);
static_assert(Awaiter<ready_awaiter<int>>);
static_assert(!Awaiter<const ready_awaiter<int>>);

static_assert(Awaitable<ready_awaitable<int>>);
static_assert(Awaitable<const ready_awaitable<int>>);
static_assert(Awaitable<const ready_awaitable<int>&>);
static_assert(!Awaiter<ready_awaitable<int>>);

static_assert(std::is_same_v<int, await_result_t<ready_awaiter<int>>>);
static_assert(std::is_same_v<int&&, await_result_t<ready_awaiter<int&&>>>);

static_assert(std::is_same_v<ready_awaiter<int&&>, awaiter_type_t<ready_awaitable<int>>>);

static_assert(std::is_same_v<int&&, await_result_t<ready_awaitable<int>>>);
static_assert(std::is_same_v<int&, await_result_t<ready_awaitable<int>&>>);
static_assert(std::is_same_v<const int&, await_result_t<const ready_awaitable<int>&>>);

static_assert(Awaitable<simple_task>);
static_assert(Awaitable<simple_task&&>);
static_assert(!Awaitable<const simple_task&>);
static_assert(!Awaitable<simple_task&>);

static_assert(AwaitableOf<simple_task, std::string>);
static_assert(AwaitableOf<simple_task, std::string_view>);
static_assert(!AwaitableOf<simple_task, int>);

struct contextually_convertible_to_bool {
    explicit operator bool();
};

struct tricky_awaiter1 {
    contextually_convertible_to_bool await_ready();
    void await_suspend(std::experimental::coroutine_handle<void>);
    void await_resume();
};

static_assert(Awaiter<tricky_awaiter1>);
static_assert(AwaiterOf<tricky_awaiter1, void>);

struct tricky_awaiter2 {
    bool await_ready();
    contextually_convertible_to_bool await_suspend(std::experimental::coroutine_handle<void>);
    void await_resume();
};

// await_suspend() must return 'bool' not 'contextually_convertible_to_bool'.
static_assert(!Awaiter<tricky_awaiter2>);
