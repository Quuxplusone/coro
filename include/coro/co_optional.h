#ifndef INCLUDED_CORO_CO_OPTIONAL_H
#define INCLUDED_CORO_CO_OPTIONAL_H

// Original source:
// https://github.com/toby-allsopp/coroutine_monad/blob/6c27a9b/maybe.h

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

#include <optional>
#include <utility>

template<class T>
struct return_object_holder {
    std::optional<T> stage_;
    return_object_holder **p_;

    explicit return_object_holder(return_object_holder **p) : p_(p) { *p_ = this; }

    return_object_holder(return_object_holder&& other) :
        stage_(std::move(other.stage_)), p_(other.p_)
    {
        *p_ = this;
    }

    return_object_holder(const return_object_holder&) = delete;
    void operator=(return_object_holder&&) = delete;
    void operator=(const return_object_holder&) = delete;

    // https://bugs.llvm.org/show_bug.cgi?id=28593
    ~return_object_holder() {}

    template<class... Args>
    void emplace(Args&&... args) {
        stage_.emplace(static_cast<Args&&>(args)...);
    }

    // This should be &&-qualified, but that requires P1155 "More implicit moves" first
    operator T() {
        return *std::move(stage_);
    }
};

template<class T>
class co_optional {
    std::optional<T> o_;

    struct maybe_promise {
        return_object_holder<co_optional> *data;

        using is_maybe_promise = void;

        auto get_return_object() { return return_object_holder<co_optional>(&data); }
        auto initial_suspend() { return std::suspend_never{}; }
        auto final_suspend() noexcept { return std::suspend_never{}; }

        void return_value(T x) { data->emplace(std::move(x)); }
        void unhandled_exception() {}
    };

    class maybe_awaitable {
        std::optional<T> o_;
    public:
        explicit maybe_awaitable(const co_optional& rhs) : o_(rhs.o_) {}
        explicit maybe_awaitable(co_optional&& rhs) : o_(std::move(rhs.o_)) {}
        bool await_ready() { return o_.has_value(); }
        T& await_resume() { return *o_; }

        template<class U, class = typename U::is_maybe_promise>
        void await_suspend(std::coroutine_handle<U> h) {
            h.promise().data->emplace(std::nullopt);
            h.destroy();
        }
    };
public:
    // These are the new additions.
    using promise_type = maybe_promise;
    friend maybe_awaitable operator co_await(co_optional o) {
        return maybe_awaitable(std::move(o));
    }

    // Now duplicate the constructor overload set of std::optional.
    // This is quite probably not a drop-in replacement for std::optional's
    // overload set, but it should do the trick for most sane code.

    constexpr co_optional() noexcept = default;
    constexpr co_optional(std::nullopt_t) noexcept {}
    constexpr co_optional(const co_optional&) = default;
    constexpr co_optional(co_optional&&) = default;

    template<class U = T, std::enable_if_t<
        !std::is_same_v<std::decay_t<U>, co_optional> &&
        std::is_constructible_v<T, U&&> && std::is_convertible_v<U&&, T>
        , int> = 0>
    constexpr co_optional(U&& u) : o_(std::in_place, static_cast<U&&>(u)) {}

    template<class U, std::enable_if_t<
        !std::is_same_v<std::decay_t<U>, co_optional> &&
        std::is_constructible_v<T, U&&> && !std::is_convertible_v<U&&, T>
        , int> = 0>
    constexpr explicit co_optional(U&& u) : o_(std::in_place, static_cast<U&&>(u)) {}

    template<class... Us, std::enable_if_t<std::is_constructible_v<T, Us&&...>, int> = 0>
    constexpr explicit co_optional(std::in_place_t, Us&&... us) : o_(std::in_place, static_cast<Us&&>(us)...) {}

    // Now duplicate the public API of std::optional.

    constexpr const T *operator->() const { return o_.operator->(); }
    constexpr T *operator->() { return o_.operator->(); }
    constexpr const T& operator*() const& { return *o_; }
    constexpr T& operator*() & { return *o_; }
    constexpr const T&& operator*() const&& { *std::move(o_); }
    constexpr T&& operator*() && { return *std::move(o_); }
    constexpr explicit operator bool() const noexcept { return o_.has_value(); }
    constexpr bool has_value() const noexcept { return o_.has_value(); }
    constexpr T& value() & { return o_.value(); }
    constexpr const T& value() const & { return o_.value(); }
    constexpr T&& value() && { return std::move(o_).value(); }
    constexpr const T&& value() const && { return std::move(o_).value(); }
    template<class U> constexpr T value_or(U&& default_value) const& { return o_.value_or(static_cast<U&&>(default_value)); }
    template<class U> constexpr T value_or(U&& default_value) && { return std::move(o_).value_or(static_cast<U&&>(default_value)); }
    void swap(co_optional& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
    {
        o_.swap(rhs.o_);
    }
    friend void swap(co_optional& a, co_optional& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
    void reset() noexcept { o_.reset(); }

    template<class... Args>
    T& emplace(Args&&... args) { return o_.emplace(static_cast<Args&&>(args)...); }

    template<class U, class... Args, class = std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>>
    T& emplace(std::initializer_list<U> il, Args&&... args) { return o_.emplace(il, static_cast<Args&&>(args)...); }

    friend constexpr bool operator==(const co_optional& a, const co_optional& b) {
        return a.has_value() ? (b.has_value() && (*a == *b)) : !b.has_value();
    }
    friend constexpr bool operator!=(const co_optional& a, const co_optional& b) {
        return !(a == b);
    }
};

template<class T>
constexpr co_optional<std::decay_t<T>> make_co_optional(T&& value) {
    return co_optional<std::decay_t<T>>(static_cast<T&&>(value));
}

template<class T, class... Args>
constexpr co_optional<T> make_co_optional(Args&&... args) {
    return co_optional<T>(std::in_place, static_cast<Args&&>(args)...);
}

template<class T, class U, class... Args>
constexpr co_optional<T> make_co_optional(std::initializer_list<U> il, Args&&... args) {
    return co_optional<T>(std::in_place, il, static_cast<Args&&>(args)...);
}

#endif // INCLUDED_CORO_CO_OPTIONAL_H
