// https://coro.godbolt.org/z/

// Original source:

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/concepts.h>
#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/co_future.h>
#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/new_thread_context.h>
#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/sync_wait.h>
#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/task.h>
#include <chrono>
#include <iostream>
#include <thread>

template<Executor E>
auto async_count(E e, int *result) -> task<void>
{
    co_await e.schedule();
    for (int i=0; i < 10; ++i) {
        std::cout << "Thread " << std::this_thread::get_id() << " counted " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    *result = 42;
    co_return;
}

int main()
{
    new_thread_context ctx;
    auto e = ctx.get_executor();
    int i = -1;
    int j = -2;
    auto f1 = async_count(e, &i);
    auto f2 = async_count(e, &j);
    sync_wait(f1);
    sync_wait(f2);
    std::cout << i << " " << j << std::endl;
}
