// https://coro.godbolt.org/z/GHaP7v

// Original source:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2168r0.pdf

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/p2168r0_generator.h>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

generator<std::string_view> f() {
    co_yield std::string();
}

// -----------------------------------------

struct some_error {};
generator<int> might_throw() {
    co_yield 0;
    throw some_error{};
}

generator<int> nested_ints() {
    try {
        co_yield might_throw();
    } catch (const some_error&) {}
    co_yield 1;
}

// nested_ints() is semantically equivalent to the following:
generator<int> manual_ints() {
    try {
        for (int x : might_throw()) {
            co_yield x;
        }
    } catch (const some_error&) {}
    co_yield 1;
}

void consumer() {
    for (int x : nested_ints()) {
        std::cout << x << " "; // outputs 0 1
    }
    for (int x : manual_ints()) {
        std::cout << x << " "; // also outputs 0 1
    }
}

// -----------------------------------------

struct Tree {
    std::shared_ptr<Tree> left;
    std::shared_ptr<Tree> right;
    int value;

    explicit Tree(std::shared_ptr<Tree> l, std::shared_ptr<Tree> r, int v)
        : left(l), right(r), value(v) {}

    generator<int> visit() const {
        if (left) co_yield left->visit();
        co_yield value;
        if (right) co_yield right->visit();
    }
};

std::shared_ptr<Tree> make_example_tree() {
    auto t1 = std::make_shared<Tree>(nullptr, nullptr, 1);
    auto t2 = std::make_shared<Tree>(t1, nullptr, 2);
    auto t4 = std::make_shared<Tree>(nullptr, nullptr, 4);
    auto t3 = std::make_shared<Tree>(t2, t4, 3);
    auto t7 = std::make_shared<Tree>(nullptr, nullptr, 7);
    auto t6 = std::make_shared<Tree>(nullptr, t7, 6);
    auto t5 = std::make_shared<Tree>(t3, t6, 5);
    return t5;
}

void visit_a_tree() {
    auto t = make_example_tree();
    for (int x : t->visit()) {
        std::cout << x << "\n";
    }
}

// -----------------------------------------

#if __has_include(<ranges>)
#include <ranges>

template<std::ranges::input_range Rng1, std::ranges::input_range Rng2>
generator<
    std::tuple<std::ranges::range_reference_t<Rng1>, std::ranges::range_reference_t<Rng2>>,
    std::tuple<std::ranges::range_value_t<Rng1>, std::ranges::range_value_t<Rng2>>
>
zip(Rng1 r1, Rng2 r2) {
    auto it1 = std::ranges::begin(r1);
    auto it2 = std::ranges::begin(r2);
    auto end1 = std::ranges::end(r1);
    auto end2 = std::ranges::end(r2);
    while (it1 != end1 && it2 != end2) {
        co_yield {*it1, *it2};
        ++it1; ++it2;
    }
}
#endif // __has_include(<ranges>)

// -----------------------------------------

int main() {
    consumer();
    visit_a_tree();
}
