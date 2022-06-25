#include "base/simple_dynamic_string.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

int testSdsAppendSpecifyTheLength()
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello ");
    std::string_view hello = "Redis+++";
    sds.Append(hello.data(), 7);
    assert(sds == "Hello Redis++"_sds);
    std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
    return 0;
}

int testSdsAppendStringOfCStyle()
{
    using namespace base::literals;
    
    base::SimpleDynamicString sds("Hello ");
    sds.Append("Redis++");
    assert(sds == "Hello Redis++"_sds);
    std::cout << "testSdsAppendStringOfCStyle pass " << sds.Data() << std::endl;

    return 0;
}

int testSdsAppendString_view()
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello ");
    std::string_view  sv = "redis++";
    sds.Append(sv);
    assert(sds == "Hello redis++"_sds);
    std::cout << "testSdsAppendString_view pass " << sds.Data() << std::endl;
    return 0;
}

int testSdsAppendSds()
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello ");
    base::SimpleDynamicString redisPlus("redis++");
    sds.Append(redisPlus);
    assert(sds == "Hello redis++"_sds);
    std::cout << "testSdsAppendSds pass " << sds.Data() << std::endl;
    return 0;
}

int testSdsRange()
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello redis++");
    sds.Range(6, sds.Length());
    assert(sds == "redis++"_sds);
    std::cout << "testSdsRange pass " << sds.Data() << std::endl;
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
    testSdsAppendString_view();
    testSdsAppendSds();
    testSdsRange();
    return 0;
}
