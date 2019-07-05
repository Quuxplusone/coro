// https://coro.godbolt.org/z/7jquWi

// Original source:
// https://www.bfilipek.com/2018/05/using-optional.html

#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/co_optional.h>
#include <assert.h>
#include <optional>
#include <string>

auto parseInt(const std::string& input)
    -> co_optional<int>
{
    try {
        return std::stoi(input);
    } catch (...) {
        return std::nullopt;
    }
}

auto splitString(const std::string& input)
    -> co_optional<std::pair<std::string, std::string>>
{
    int k = input.find(',');
    if (k == input.npos || k != input.rfind(',')) {
        return std::nullopt;
    }
    return std::make_pair(input.substr(0, k), input.substr(k+1));
}

co_optional<int> add(co_optional<int> a, co_optional<int> b)
{
    co_return (co_await a + co_await b);
}

co_optional<int> split_and_add(const std::string& input)
{
    auto [a, b] = co_await splitString(input);
    co_return co_await parseInt(a) + co_await parseInt(b);
}

int main()
{
    assert(add(parseInt("1"), parseInt("2")) == 3);
    assert(add(parseInt("1"), parseInt("x")) == std::nullopt);
    assert(add(parseInt("x"), parseInt("2")) == std::nullopt);
    assert(add(parseInt("x"), parseInt("y")) == std::nullopt);
    assert(split_and_add("1,2") == 3);
    assert(split_and_add("1,x") == std::nullopt);
    assert(split_and_add("x,2") == std::nullopt);
    assert(split_and_add("x,x") == std::nullopt);
    assert(split_and_add("1") == std::nullopt);
    assert(split_and_add("1,2,3") == std::nullopt);
}
