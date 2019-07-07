// https://coro.godbolt.org/z/

// Original source:
// https://www.youtube.com/watch?v=ZTqHjjm86Bw&t=42m55s (James McNellis, CppCon 2016)

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/mcnellis_generator.h>
#include <stdio.h>

generator<int> integers(int first, int last)
{
    for (int i = first; i <= last; ++i) {
        co_yield i;
    }
}

int main()
{
    for (int x : integers(1, 5)) {
        printf("%d\n", x);
    }
}
