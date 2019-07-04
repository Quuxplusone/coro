# coro
This is a collection of single-header library facilities for C++2a Coroutines.

## coro/include/

### co_future.h

Provides `co_future<T>`, which is like `std::future<T>` but models `Awaitable`.

### concepts.h

Provides definitions for the concepts and type-traits from Lewis Baker's P1288R0,
based on his own reference implementation.

- `concept Awaitable`
- `concept AwaitableOf<R>`
- `concept Awaiter`
- `concept AwaiterOf<R>`
- `awaiter_type_t<T>`
- `await_result_t<T>`
- `get_awaiter(Awaitable t)`

### shared_generator.h, unique_generator.h

`unique_generator<R>` is basically equivalent to `cppcoro::generator<R>`.
It expresses unique ownership of a coroutine handle, which means it is move-only.
This is the most natural way to implement a generator object, but it does have the
downside that it is not a range-v3 `ViewableRange`.

`shared_generator<R>` is basically equivalent to range-v3's `ranges::experimental::generator<R>`.
It expresses _reference-counted_ ownership of a coroutine handle, so that it is copyable.
It is a full `ViewableRange` and interoperates correctly with range-v3.

### sync_wait.h

Lewis Baker's implementation of P1171 `sync_wait(Awaitable t)`.
Suppose you're in `main()`, and you have a `task<int>` that you've received from a coroutine.
You can't `co_await` it, because as soon as you use the `co_await` keyword you
turn into a coroutine yourself. If (and only if?) the coroutine is being executed in another thread,
then you can pass the task off to `sync_wait`.

TODO: this needs some example code!

### task.h

`task<R>` is basically equivalent to `cppcoro::task<R>`.
It models `Awaitable` (as defined in "concepts.h").

TODO: this needs some example code!

## examples/

### generate_ints.cpp

A very simple example of `unique_generator` with `co_yield`.

### generator_as_viewable_range.cpp

Similar to `generate_ints.cpp`, this example demonstrates mixing Coroutines with Ranges.
It uses `shared_generator` (which models `ranges::ViewableRange`)
and pipes the generator object through `rv::take(10)`.

### p1288r0_concepts.cpp

Lewis Baker's reference implementation of P1288R0 comes with a test suite.
This is that test suite.

### pythagorean_triples_generator.cpp

[Eric's Famous Pythagorean Triples](http://ericniebler.com/2018/12/05/standard-ranges/),
but using a `shared_generator` that `co_yield`s tuples.
This is almost identical to `generator_as_viewable_range.cpp`; it's just
a slightly more interesting application.
