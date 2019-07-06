// https://coro.godbolt.org/z/ZzxBU3

// Original source:
// https://www.youtube.com/watch?v=ZTqHjjm86Bw&t=22m18s (James McNellis, CppCon 2016)

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/resumable_thing.h>
#include <experimental/coroutine>
#include <iostream>
#include <string>

resumable_thing named_counter(std::string name)
{
    std::cout << "counter (" << name << ") was called\n";
    for (int i=1; true; ++i) {
        co_await std::experimental::suspend_always{};
        std::cout << "counter (" << name << ") resumed #" << i << "\n";
    }
}

resumable_thing broken_named_counter(std::string_view name)
{
    std::cout << "counter (" << name << ") was called\n";
    for (int i=1; true; ++i) {
        co_await std::experimental::suspend_always{};
        std::cout << "counter (" << name << ") resumed #" << i << "\n";
    }
}

int main() {
    using namespace std::literals;
    resumable_thing counter_a = named_counter("a"s);
    resumable_thing counter_b = named_counter("b"s);
    counter_a.resume();
    counter_b.resume();
    counter_b.resume();
    counter_a.resume();
    std::cout << "main: done\n";
}
