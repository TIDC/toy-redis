#include "base/zipmap.hpp"
#include "gtest/gtest.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

TEST(SDS, Zipmap)
{
    base::Zipmap zmap;
    zmap.Set("hi\0", "this is hello hah!\0");
    zmap.Get();
    // FIXME: 字面量字符串不能直接比较
    // ASSERT_EQ("hello" == "Hello Redis++", false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}
