// https://coro.godbolt.org/z/dqWhTP

// Original source:
// https://www.youtube.com/watch?v=ZTqHjjm86Bw&t=18m00s (James McNellis, CppCon 2016)

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/resumable_thing.h>
#include <stdio.h>

resumable_thing counter()
{
    puts("counter: called");
    for (int i=1; true; ++i) {
        co_await std::experimental::suspend_always{};
        printf("counter: resumed (#%d)\n", i);
    }
}

int main() {
    puts("main: calling counter");
    auto g = counter();
    puts("main: resuming counter");
    g.resume();
    g.resume();
    puts("main: done");
}
