// https://coro.godbolt.org/z/9TWEoT

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
static_assert(ranges::range<G>);
static_assert(ranges::default_constructible<G>);
static_assert(ranges::copyable<G>);
static_assert(ranges::semiregular<G>);
static_assert(ranges::viewable_range<G>);

namespace rv = ranges::view;

int main()
{
    for (int i : ints() | rv::take(10)) {
        printf("%d\n", i);
    }
    printf("done\n");
}
