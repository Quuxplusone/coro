// https://coro.godbolt.org/z/GGDB9B

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/shared_generator.h>
#include <stdio.h>
#include <range/v3/view/take.hpp>

shared_generator<int> ints()
{
    int i = 0;
    while (true)
        co_yield i++;
}

using G = decltype(ints());
static_assert(ranges::Range<G>);
static_assert(ranges::DefaultConstructible<G>);
static_assert(ranges::Copyable<G>);
static_assert(ranges::Semiregular<G>);
static_assert(ranges::ViewableRange<G>);

namespace rv = ranges::view;

int main()
{
    for (int i : ints() | rv::take(10)) {
        printf("%d\n", i);
    }
    printf("done\n");
}
