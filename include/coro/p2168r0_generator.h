#ifndef INCLUDED_CORO_P2168R0_GENERATOR_H
#define INCLUDED_CORO_P2168R0_GENERATOR_H

// Original source:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2168r0.pdf
// https://godbolt.org/z/icfqLr (Lewis Baker)

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

#include <cstdio>
#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>

template<typename Ref, typename Value = std::remove_cvref_t<Ref>>
class generator {
public:
    class promise_type {

        using YieldType = std::conditional_t<
            std::is_reference_v<Ref>,
            Ref&&,
            const Ref&
        >;

    public:
        explicit promise_type() : rootleaf_(this) {}
        generator get_return_object() noexcept {
            return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        void unhandled_exception() {
            if (exception_ == nullptr)
                throw;
            *exception_ = std::current_exception();
        }

        void return_void() noexcept {}

        std::suspend_always initial_suspend() noexcept { return {}; }

        // Transfers control back to the parent of a nested coroutine
        struct final_awaiter {
            bool await_ready() noexcept {
                return false;
            }
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                auto& promise  = h.promise();
                auto  parent   = h.promise().parent_;
                if (parent) {
                    promise.rootleaf_->rootleaf_ = parent;
                    return std::coroutine_handle<promise_type>::from_promise(*parent);
                }
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(YieldType x) noexcept {
            rootleaf_->value_ = std::addressof(x);
            return {};
        }

        struct yield_sequence_awaiter {
            generator gen_;
            std::exception_ptr exception_;

            explicit yield_sequence_awaiter(generator&& g) noexcept
            // Taking ownership of the generator ensures frames are destroyed
            // in the reverse order of their creation
            : gen_(std::move(g))
            {}

            bool await_ready() noexcept {
                return !gen_.coro_;
            }

            // set the parent, root and exceptions pointer and
            // resume the nested coroutine
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                auto& current = h.promise();
                auto& nested = gen_.coro_.promise();
                auto& root = current.rootleaf_;
                
                nested.rootleaf_ = root;
                root->rootleaf_ = &nested;
                nested.parent_ = &current;

                nested.exception_  = &exception_;
                
                // Immediately resume the nested coroutine (nested generator)
                return gen_.coro_;
            }

            void await_resume() {
                if (exception_) {
                    std::rethrow_exception(std::move(exception_));
                }
            }
        };

        yield_sequence_awaiter yield_value(generator&& g) noexcept {
            return yield_sequence_awaiter{std::move(g)};
        }

        void resume() {
            std::coroutine_handle<promise_type>::from_promise(*rootleaf_).resume();
        }

        // Disable use of co_await within this coroutine.
        void await_transform() = delete;

    private:
        friend generator;
        
        promise_type *rootleaf_ = nullptr;
        promise_type *parent_ = nullptr;
        std::exception_ptr *exception_ = nullptr;
        std::add_pointer_t<const Ref> value_;
    };

    generator() noexcept = default;

    generator(generator&& other) noexcept
        : coro_(std::exchange(other.coro_, {}))
    {}

    ~generator() {
        if (coro_) {
            coro_.destroy();
        }
    }

    generator& operator=(generator g) noexcept {
        swap(g);
        return *this;
    }

    void swap(generator& other) noexcept {
        std::swap(coro_, other.coro_);
    }

    struct sentinel {};

    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Value;
        using reference = Ref;
        using pointer = std::add_pointer_t<Ref>;

        iterator() noexcept = default;
        iterator(const iterator&) = delete;
        iterator(iterator&& o) : coro_(std::exchange(o.coro_, {})) {}
        iterator& operator=(iterator o) { std::swap(coro_, o.coro_); return *this; }
        ~iterator() = default;

        friend bool operator==(const iterator &it, sentinel) noexcept {
            return !it.coro_ || it.coro_.done();
        }

        iterator& operator++() {
            coro_.promise().resume();
            return *this;
        }
        void operator++(int) {
            (void)operator++();
        }

        reference operator*() const noexcept {
            return static_cast<reference>(*coro_.promise().value_);
        }

        pointer operator->() const noexcept 
            requires std::is_reference_v<reference>
        {
            return std::addressof(operator*());
        }

      private:
        friend generator;

        explicit iterator(std::coroutine_handle<promise_type> coro) noexcept
            : coro_(coro) {}

        std::coroutine_handle<promise_type> coro_;
    };

    iterator begin() {
        if (coro_) {
            coro_.resume();
        }
        return iterator{coro_};
    }

    sentinel end() noexcept { return sentinel{}; }

private:
    explicit generator(std::coroutine_handle<promise_type> coro) noexcept
        : coro_(coro) {}

    std::coroutine_handle<promise_type> coro_;
};

#if __has_include(<ranges>)
#include <ranges>
template<class T, class U>
constexpr inline bool std::ranges::enable_view<generator<T, U>> = true;
#endif // __has_include(<ranges>)

#endif // INCLUDED_CORO_P2168R0_GENERATOR_H
