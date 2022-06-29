#include "base/dictionary.hpp"
#include "base/hello.h"

#include <iostream>

#if !defined(__cpp_coroutines) && defined(__cpp_impl_coroutine)
#include <coroutine>
struct promise;
struct coroutine : std::coroutine_handle<promise>
{
    using promise_type = struct promise;
};

struct promise
{
    coroutine get_return_object()
    {
        return {coroutine::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept
    {
        return {};
    }
    std::suspend_always final_suspend() noexcept
    {
        return {};
    }
    void return_void() {}
    void unhandled_exception() {}
};

void bad3()
{
    coroutine h = [i = 0]() -> coroutine { // a lambda that's also a coroutine
        std::cout << i;
        co_return;
    }(); // immediately invoked
    // lambda destroyed
    h.resume(); // uses (anonymous lambda type)::i after free
    h.destroy();
}
#else
void bad3() {}
#endif

int main(int, char **)
{
    bad3();
    std::cout << HelloMessage() << std::endl;
}
