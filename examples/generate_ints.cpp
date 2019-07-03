// https://coro.godbolt.org/z/1ILjMu

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/unique_generator.h>
#include <stdio.h>

unique_generator<int> ints(int n)
{
    for (int i = 0; i < n; ++i) {
        co_yield i;
    }
}

int main()
{
    for (int i : ints(10)) {
        printf("%d\n", i);
    }
    printf("done\n");
}
