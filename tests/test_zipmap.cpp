#include "base/zipmap.hpp"
#include "gtest/gtest.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

TEST(SDS, ZipMap)
{
    base::ZipMap zmap;
    zmap.Set("hi\0", "this is hello hah!\0");
    if (auto value = zmap.Get(std::string_view("")))
    {
        std::cout << "create2(true) returned " << *value << '\n';
    }
    std::cout << "length " << zmap.Length() << std::endl;
    std::cout << "size " << zmap.Size() << std::endl;

    // FIXME: 字面量字符串不能直接比较
    // ASSERT_EQ("hello" == "Hello Redis++", false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}
