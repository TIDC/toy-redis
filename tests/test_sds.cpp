#include "base/simple_dynamic_string.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

int testSdsAppend()
{
    std::string_view hello = "Hello Redis++";
    base::SimpleDynamicString sds(hello);
    uint64_t  oldSDS = sds.Length();
    sds.Append(hello.data(), hello.length());
    std::cout << sds.Data() << std::endl;
    assert(sds.Length() == oldSDS + hello.length());
    return 0;
}

int main(int argc, const char *args[])
{
    std::string_view hello = "Hello World";
    base::SimpleDynamicString sds(hello);

    assert(sds.Length() == hello.length());
    std::cout << sds.Length() << std::endl;
    testSdsAppend();
    return 0;
}
