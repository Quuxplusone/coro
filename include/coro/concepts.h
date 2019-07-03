#ifndef INCLUDED_CORO_CONCEPTS_H
#define INCLUDED_CORO_CONCEPTS_H

// Original source:
// http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1288r0.pdf
// https://godbolt.org/z/9dapP6

#include <experimental/coroutine>
#include <type_traits>

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_concepts) && __cpp_lib_concepts >= 201806L && __has_include(<concepts>)

#include <concepts>

#else

namespace std {
    // Awaitable refines Movable
    template<class T> concept Movable = std::is_move_constructible_v<T>;
}

#endif

namespace std::experimental {
    template<class> struct __is_valid_await_suspend_return_type : false_type {};
    template<> struct __is_valid_await_suspend_return_type<bool> : true_type {};
    template<> struct __is_valid_await_suspend_return_type<void> : true_type {};
    template<class P> struct __is_valid_await_suspend_return_type<coroutine_handle<P>> : true_type {};

    template<class T>
    concept Awaiter = requires(T&& awaiter, coroutine_handle<void> h) {
        { awaiter.await_ready() ? void() : void() };  // test contextually-convertible-to-bool-ness
        { awaiter.await_suspend(h) };
        { awaiter.await_resume() };
        requires __is_valid_await_suspend_return_type<decltype( awaiter.await_suspend(h) )>::value;
    };


    template<class T, class Result>
    concept AwaiterOf = Awaiter<T> && requires(T&& awaiter) {
        requires std::is_convertible_v<decltype( awaiter.await_resume() ), Result>;
        requires std::is_void_v<Result> || std::is_constructible_v<Result, decltype( awaiter.await_resume() )>;
    };

    // get_awaiter
    namespace get_awaiter_detail {
        struct NoneSuch {};
        void operator co_await(NoneSuch);  // hide non-ADL versions of co_await

        template<int I> struct priority_tag : priority_tag<I-1> {};
        template<> struct priority_tag<0> {};

        template<class T> auto get_awaiter(T&& t, priority_tag<2>)
            noexcept(noexcept( static_cast<T&&>(t).operator co_await() ))
            -> decltype( static_cast<T&&>(t).operator co_await() )
        {
            return static_cast<T&&>(t).operator co_await();
        }

        template<class T> auto get_awaiter(T&& t, priority_tag<1>)
            noexcept(noexcept( operator co_await(static_cast<T&&>(t)) ))
            -> decltype( operator co_await(static_cast<T&&>(t)) )
        {
            return operator co_await(static_cast<T&&>(t));
        }

        template<class T>
        decltype(auto) get_awaiter(T&& t, priority_tag<0>) noexcept
        {
            return static_cast<T&&>(t);
        }
    }

    // get_awaiter(x) always exists, but the type of get_awaiter(x) will
    // satisfy Awaiter only when x is Awaitable.

    template<class T>
    decltype(auto) get_awaiter(T&& t)
        noexcept(noexcept( get_awaiter_detail::get_awaiter(static_cast<T&&>(t), get_awaiter_detail::priority_tag<2>{}) ))
    {
        return get_awaiter_detail::get_awaiter(static_cast<T&&>(t), get_awaiter_detail::priority_tag<2>{});
    }

    template<class T>
    struct awaiter_type {
        using type = decltype( std::experimental::get_awaiter(std::declval<T>()) );
    };
    template<class T> using awaiter_type_t = typename awaiter_type<T>::type;

    template<class T>
    concept Awaitable = std::Movable<T> &&
        Awaiter<decltype( std::experimental::get_awaiter(std::declval<T&&>()) )>;

    template<class T, class Result>
    concept AwaitableOf = Awaitable<T> &&
        AwaiterOf<decltype( std::experimental::get_awaiter(std::declval<T&&>()) ), Result>;

    // await_result_t

    template<Awaitable T> struct await_result {
        using type = decltype( std::declval<awaiter_type_t<T>&>().await_resume() );
    };
    template<Awaitable T> using await_result_t = typename await_result<T>::type;

} // namespace std::experimental

#endif // INCLUDED_CORO_CONCEPTS_H
