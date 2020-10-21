#ifndef INCLUDED_CORO_CONCEPTS_H
#define INCLUDED_CORO_CONCEPTS_H

// Original source:
// http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1288r0.pdf
// https://godbolt.org/z/9dapP6
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1341r0.pdf

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

#include <type_traits>

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_concepts) && __cpp_lib_concepts >= 201806L && __has_include(<concepts>)

#include <concepts>

#else

namespace std {
    template<class From, class To>
    concept ConvertibleTo = std::is_convertible_v<From, To> && requires (From (&f)()) {
        static_cast<To>(f());
    };

    template<class T>
    concept Destructible = std::is_nothrow_destructible_v<T>;

    template<class T, class... Args>
    concept Constructible = Destructible<T> && std::is_constructible_v<T, Args...>;

    template<class T>
    concept DefaultConstructible = Constructible<T>;

    template<class T>
    concept MoveConstructible = Constructible<T, T> && ConvertibleTo<T, T>;

    template<class T>
    concept CopyConstructible = MoveConstructible<T> &&
        Constructible<T, T&> && ConvertibleTo<T&, T> &&
        Constructible<T, const T&> && ConvertibleTo<const T&, T> &&
        Constructible<T, const T> && ConvertibleTo<const T, T>;

    template<class LHS, class RHS>
    concept Assignable =
        std::is_lvalue_reference_v<LHS> &&
        /*CommonReference<const std::remove_reference_t<LHS>&, const std::remove_reference_t<RHS>&> &&*/
        std::is_same_v<LHS, decltype( std::declval<LHS&>() = std::declval<RHS&&>() )>;

    template<class T, class U>
    concept ExpositionOnlyWeaklyEqualityComparableWith =
    requires(const std::remove_reference_t<T>& t, const std::remove_reference_t<U>& u) {
        // Don't implement http://eel.is/c++draft/concept.boolean because it is insane
        requires ConvertibleTo<decltype(t == u), bool>;
        requires ConvertibleTo<decltype(t != u), bool>;
        requires ConvertibleTo<decltype(u == t), bool>;
        requires ConvertibleTo<decltype(u != t), bool>;
    };

    template<class T>
    concept EqualityComparable = ExpositionOnlyWeaklyEqualityComparableWith<T, T>;

    template<class T>
    concept Movable = std::is_object_v<T> && MoveConstructible<T> && Assignable<T&, T> /*&& Swappable<T>*/;

    template<class T>
    concept Copyable = CopyConstructible<T> && Movable<T> && Assignable<T&, const T&>;

    template<class T>
    concept Semiregular = Copyable<T> && DefaultConstructible<T>;

    template<class T>
    concept Regular = Semiregular<T> && EqualityComparable<T>;
}

#endif

template<class> struct __is_valid_await_suspend_return_type : std::false_type {};
template<> struct __is_valid_await_suspend_return_type<bool> : std::true_type {};
template<> struct __is_valid_await_suspend_return_type<void> : std::true_type {};
template<class P> struct __is_valid_await_suspend_return_type<std::coroutine_handle<P>> : std::true_type {};

template<class T>
concept Awaiter = requires(T&& awaiter, std::coroutine_handle<void> h) {
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
    using type = decltype( ::get_awaiter(std::declval<T>()) );
};
template<class T> using awaiter_type_t = typename awaiter_type<T>::type;

template<class T>
concept Awaitable = std::MoveConstructible<T> &&
    Awaiter<decltype( ::get_awaiter(std::declval<T&&>()) )>;

template<class T, class Result>
concept AwaitableOf = Awaitable<T> &&
    AwaiterOf<decltype( ::get_awaiter(std::declval<T&&>()) ), Result>;

// await_result_t

template<Awaitable T> struct await_result {
    using type = decltype( std::declval<awaiter_type_t<T>&>().await_resume() );
};
template<Awaitable T> using await_result_t = typename await_result<T>::type;

// End of P1288R0. Beginning of P1341R0.

template<class T>
concept Executor =
    std::CopyConstructible<T> &&
    std::is_nothrow_move_constructible_v<T> &&
    Awaitable<decltype( std::declval<T&>().schedule() )>;



#endif // INCLUDED_CORO_CONCEPTS_H
