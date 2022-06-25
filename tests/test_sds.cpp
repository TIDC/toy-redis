#include "base/simple_dynamic_string.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

int testSdsAppendSpecifyTheLength()
{
    std::string_view hello = "Hello Redis++";
    base::SimpleDynamicString sds(hello);
    uint64_t oldSDS = sds.Length();
    sds.Append(hello.data(), hello.length());
    assert(sds.Length() == oldSDS + hello.length());
    std::cout << sds.Data() << std::endl;
    return 0;
}

int testSdsAppendStringOfCStyle()
{
    using namespace base::literals;
    
    base::SimpleDynamicString sds("Hello ");
    sds.Append("Redis++");
    assert(strcmp(sds.Data(), "Hello Redis++") == 0);
    std::cout << sds.Data() << std::endl;

    assert(sds == "Hello Redis++"_sds);

    return 0;
}

int main(int argc, const char *args[])
{
    std::string_view hello = "Hello World";
    base::SimpleDynamicString sds(hello);

    assert(sds.Length() == hello.length());
    std::cout << sds.Length() << std::endl;
    testSdsAppendSpecifyTheLength();
    testSdsAppendStringOfCStyle();
    return 0;
}
