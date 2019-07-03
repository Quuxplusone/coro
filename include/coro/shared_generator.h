#ifndef INCLUDED_CORO_SHARED_GENERATOR_H
#define INCLUDED_CORO_SHARED_GENERATOR_H

// Original source:
// https://github.com/lewissbaker/llvm/blob/9f59dcce/coroutine_examples/manual_lifetime.hpp
// https://github.com/lewissbaker/llvm/blob/9f59dcce/coroutine_examples/generator.hpp
// https://github.com/ericniebler/range-v3/blob/664aa80/include/range/v3/experimental/utility/generator.hpp

#include <atomic>
#include <experimental/coroutine>
#include <iterator>
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

    void destruct() noexcept {
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
    manual_lifetime() noexcept = default;

    void construct(T& value) noexcept {
        ptr = std::addressof(value);
    }
    void destruct() noexcept {
        ptr = nullptr;
    }

    T& get() const noexcept { return *ptr; }

private:
    T *ptr = nullptr;
};

template<class T>
struct manual_lifetime<T&&> {
    manual_lifetime() noexcept = default;

    void construct(T&& value) noexcept {
        ptr = std::addressof(value);
    }
    void destruct() noexcept {
        ptr = nullptr;
    }

    T&& get() const noexcept { return *ptr; }

private:
    T *ptr = nullptr;
};

template<>
struct manual_lifetime<void> {
    void construct() noexcept {}
    void destruct() noexcept {}
    void get() const noexcept {}
};

#endif // INCLUDED_CORO_MANUAL_LIFETIME_H

template<class Ref, class Value = std::decay_t<Ref>>
class shared_generator {
public:
    class promise_type {
    public:
        promise_type() noexcept {}

        ~promise_type() noexcept {
            clear_value();
        }

        void clear_value() {
            if (hasValue_) {
                hasValue_ = false;
                ref_.destruct();
            }
        }

        shared_generator get_return_object() noexcept {
            return shared_generator{
                std::experimental::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        auto initial_suspend() noexcept {
            return std::experimental::suspend_always{};
        }

        auto final_suspend() noexcept {
            return std::experimental::suspend_always{};
        }

        auto yield_value(Ref ref)
                noexcept(std::is_nothrow_move_constructible_v<Ref>) {
            ref_.construct(std::move(ref));
            return std::experimental::suspend_always{};
        }

        void return_void() {}

        void unhandled_exception() {
            throw;
        }

        Ref get() {
            return ref_.get();
        }

    private:
        friend class shared_generator;
        manual_lifetime<Ref> ref_;
        bool hasValue_ = false;
        std::atomic<int> refcount_{1};
    };

    using handle_t = std::experimental::coroutine_handle<promise_type>;

    // ViewableRange refines Semiregular refines DefaultConstructible
    explicit shared_generator() {}

    shared_generator(shared_generator&& g) noexcept :
        coro_(std::exchange(g.coro_, {}))
    {}

    // ViewableRange refines Semiregular refines Copyable
    shared_generator(const shared_generator& g) noexcept :
        coro_(g.coro_)
    {
        if (coro_) {
            ++coro_.promise().refcount_;
        }
    }

    // ViewableRange refines Semiregular refines Copyable
    shared_generator& operator=(shared_generator g) noexcept {
        this->swap(g);
    }

    void swap(shared_generator& g) noexcept {
        using std::swap;
        swap(coro_, g.coro_);
    }

    friend void swap(shared_generator& g, shared_generator& h) noexcept {
        g.swap(h);
    }

    ~shared_generator() {
        if (coro_) {
            if (--coro_.promise().refcount_ == 0) {
                coro_.destroy();
            }
        }
    }

    struct sentinel {};

    class iterator {
    public:
        using reference = Ref;
        using value_type = Value;
        using difference_type = std::ptrdiff_t;
        using pointer = std::add_pointer_t<Ref>;
        using iterator_category = std::input_iterator_tag;

        iterator() noexcept {}

        explicit iterator(handle_t coro) noexcept :
            coro_(coro)
        {}

        reference operator*() const {
            return coro_.promise().get();
        }

        iterator& operator++() {
            coro_.promise().clear_value();
            coro_.resume();
            return *this;
        }

        void operator++(int) {
            coro_.promise().clear_value();
            coro_.resume();
        }

        friend bool operator==(const iterator& it, sentinel) noexcept { return it.coro_.done(); }
        friend bool operator==(sentinel, const iterator& it) noexcept { return it.coro_.done(); }
        friend bool operator!=(const iterator& it, sentinel) noexcept { return !it.coro_.done(); }
        friend bool operator!=(sentinel, const iterator& it) noexcept { return !it.coro_.done(); }

    private:
        handle_t coro_;
    };

    iterator begin() {
        coro_.resume();
        return iterator{coro_};
    }

    sentinel end() {
        return {};
    }

private:
    explicit shared_generator(handle_t coro) noexcept :
        coro_(coro)
    {}

    handle_t coro_;
};

#endif // INCLUDED_CORO_SHARED_GENERATOR_H
