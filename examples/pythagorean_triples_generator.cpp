// https://coro.godbolt.org/z/4rzMI9

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/shared_generator.h>
#include <stdio.h>
#include <tuple>
#include <range/v3/view/take.hpp>

namespace rv = ranges::view;

auto triples() -> shared_generator<std::tuple<int, int, int>> { 
    for (int z = 1; true; ++z) {
        for (int x = 1; x < z; ++x) {
            for (int y = x; y < z; ++y) {
                if (x*x + y*y == z*z) {
                    co_yield std::make_tuple(x, y, z);
                }
            }
        }
    }
}

int main() {
    for (auto&& triple : triples() | rv::take(10)) {
        printf(
            "(%d,%d,%d)\n",
            std::get<0>(triple),
            std::get<1>(triple),
            std::get<2>(triple)
        );
    }
}
