// https://coro.godbolt.org/z/s_nA-K

// Original source:
// https://www.youtube.com/watch?v=8C8NnE1Dg4A&t=6m00s (Gor Nishanov, CppCon 2016)

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/mcnellis_generator.h>
#include <numeric>
#include <stdio.h>

generator<int> gen() {
    for (int i = 0; true; ++i) {
        co_yield i;
    }
}

generator<int> take_until(generator<int>& g, int sentinel) {
    for (int v : g) {
        if (v == sentinel) {
            break;
        }
        co_yield v;
    }
}

int main() {
    auto g = gen();
    auto t = take_until(g, 10);
    int r = std::accumulate(t.begin(), t.end(), 0);
    printf("%d\n", r);
}
