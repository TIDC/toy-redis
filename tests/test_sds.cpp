#include "base/simple_dynamic_string.hpp"
#include "gtest/gtest.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

TEST(SDS, AppendSpecifyTheLength)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello ");
    std::string_view hello = "Redis+++";
    sds.Append(hello.data(), 7);
    ASSERT_EQ(sds == "Hello Redis++"_sds, true);
    std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}

TEST(SDS, AppendStringOfCStyle)
{
    using namespace base::literals;

    base::SimpleDynamicString sds("Hello ");
    sds.Append("Redis++");
    ASSERT_EQ(sds == "Hello Redis++"_sds, true);
    std::cout << "testSdsAppendStringOfCStyle pass " << sds.Data() << std::endl;
}

TEST(SDS, AppendStringView)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello ");
    std::string_view sv = "redis++";
    sds.Append(sv);
    ASSERT_EQ(sds == "Hello redis++"_sds, true);
    std::cout << "testSdsAppendString_view pass " << sds.Data() << std::endl;
}

TEST(SDS, AppendSds)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello ");
    base::SimpleDynamicString redisPlus("redis++");
    sds.Append(redisPlus);
    ASSERT_EQ(sds == "Hello redis++"_sds, true);
    std::cout << "testSdsAppendSds pass " << sds.Data() << std::endl;
}

TEST(SDS, SdsRange)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("Hello redis++");
    sds.Range(6, sds.Length());
    ASSERT_EQ(sds == "redis++"_sds, true);
    std::cout << "testSdsRange pass " << sds.Data() << std::endl;
}

TEST(SDS, SdsTrim)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("rrHello redis++rr");
    sds.Trim("r");
    std::cout << sds.Data() << std::endl;
    ASSERT_EQ(sds == "Hello redis++"_sds, true);
    std::cout << "Trim pass " << sds.Data() << std::endl;
}

TEST(SDS, SdsTrim2)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("rrHello redis++rrrr");
    sds.Trim2('r');
    std::cout << sds.Data() << std::endl;
    ASSERT_EQ(sds == "Hello redis++"_sds, true);
    std::cout << "Trim2 pass " << sds.Data() << std::endl;
}

TEST(SDS, SdsToLower)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("HELLO");
    sds.ToLower();
    std::cout << sds.Data() << std::endl;
    ASSERT_EQ(sds == "hello"_sds, true);
    std::cout << "ToLower pass " << sds.Data() << std::endl;
}

TEST(SDS, SdsToUpper)
{
    using namespace base::literals;
    base::SimpleDynamicString sds("hello");
    sds.ToUpper();
    std::cout << sds.Data() << std::endl;
    ASSERT_EQ(sds == "HELLO"_sds, true);
    std::cout << "ToUpper pass " << sds.Data() << std::endl;
}