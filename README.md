# coro
This is a collection of single-header library facilities for C++2a Coroutines.

## coro/include/

### co_future.h

Provides `co_future<T>`, which is like `std::future<T>` but models `Awaitable`.

### co_optional.h

Provides `co_optional<T>`, which is like `std::optional<T>` but is awaitable.
This is based on [code originally by Toby Allsopp](https://github.com/toby-allsopp/coroutine_monad/#using-coroutines-for-monadic-composition).
Notice that `co_optional<T>` is awaitable only in coroutine contexts where the
return type is itself `co_optional<U>` — not, say, `task<U>` or `generator<U>` —
and therefore `co_optional<T>` does _not_ model the P1288R0 `Awaitable` concept.

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

### gor_generator.h

Gor Nishanov's `generator<R>`. The difference between this one and "mcnellis_generator.h"
is that this one stores the value of `coro_.done()` in a bool member variable. That change
makes it more friendly to the compiler's optimizer. This is the only generator that works
as intended with "disappearing_coroutine.cpp".

This generator is move-only.

### mcnellis_generator.h

James McNellis's `int_generator` example from "Introduction to C++ Coroutines" (CppCon 2016),
templatized into `generator<T>`. I had to fill in some boilerplate he didn't show, such
as iterator comparison, `return_void`/`unhandled_exception`, and several constructors.
Any mistakes are likely mine, not his.

This generator is neither moveable nor copyable.

### shared_generator.h, unique_generator.h

`unique_generator<R>` is basically equivalent to `cppcoro::generator<R>`.
It expresses unique ownership of a coroutine handle, which means it is move-only.
This is the most natural way to implement a generator object, but it does have the
downside that it is not a range-v3 `viewable_range`.

`shared_generator<R>` is basically equivalent to range-v3's `ranges::experimental::generator<R>`.
It expresses _reference-counted_ ownership of a coroutine handle, so that it is copyable.
It is a full `viewable_range` and interoperates correctly with range-v3.

These generators' `end()` methods return a sentinel type instead of `iterator`,
which means that these generators do not interoperate with the C++17 STL algorithms.

### resumable_thing.h

James McNellis's `resumable_thing` example from "Introduction to C++ Coroutines" (CppCon 2016).

### sync_wait.h

Lewis Baker's implementation of P1171 `sync_wait(Awaitable t)`.
Suppose you're in `main()`, and you have a `task<int>` that you've received from a coroutine.
You can't `co_await` it, because as soon as you use the `co_await` keyword you
turn into a coroutine yourself. If (and only if?) the coroutine is being executed in another thread,
then you can pass the task off to `sync_wait`.

TODO: this needs some example code!

### task.h, gor_task.h

`task<R>` is basically equivalent to `cppcoro::task<R>`.
It models `Awaitable` (as defined in "concepts.h").

"gor_task.h" provides another implementation of `task<R>`, as shown in Gor Nishanov's
"C++ Coroutines: Under the Covers" (CppCon 2016).

TODO: this needs some example code!

## examples/

### co_optional.cpp

Simple examples of using `co_optional` monadic operations with `co_await` and `co_return`.

### co_optional_tests.cpp

Toby Allsopp's monadic `optional` comes with a test suite.
This is that test suite.

### disappearing_coroutine.cpp

Gor Nishanov's example of passing a generator to `std::accumulate`, from
his talk "C++ Coroutines: Under the Covers" (CppCon 2016). Clang can optimize
this down to a single `printf`; but only if you use "gor_generator.h". If you use
one of the generators that doesn't cache `coro_.done()` in a data member, Clang will
not be able to optimize it.

### generate_ints.cpp

A very simple example of `unique_generator` with `co_yield`.

### generator_as_viewable_range.cpp

Similar to `generate_ints.cpp`, this example demonstrates mixing Coroutines with Ranges.
It uses `shared_generator` (which models `ranges::viewable_range`)
and pipes the generator object through `rv::take(10)`.

### mcnellis_generator.cpp

James McNellis's `int_generator` example from "Introduction to C++ Coroutines" (CppCon 2016).

### mcnellis_resumable_thing.cpp

James McNellis's first `resumable_thing` example from "Introduction to C++ Coroutines" (CppCon 2016).

### mcnellis_resumable_thing_dangling.cpp

James McNellis's second `resumable_thing` example from "Introduction to C++ Coroutines" (CppCon 2016),
showing the interleaved execution of two `named_counter` coroutines.

We show both McNellis's working `named_counter`, which captures a `std::string` by value,
_and_ a `broken_named_counter` that captures a `std::string_view` and thus suffers from a
subtle dangling-reference bug whose effects are visible in the output.

### p1288r0_concepts.cpp

Lewis Baker's reference implementation of P1288R0 comes with a test suite.
This is that test suite.

### pythagorean_triples_generator.cpp

[Eric's Famous Pythagorean Triples](http://ericniebler.com/2018/12/05/standard-ranges/),
but using a `shared_generator` that `co_yield`s tuples.
This is almost identical to `generator_as_viewable_range.cpp`; it's just
a slightly more interesting application.
