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
    ASSERT_EQ("hello" == "Hello Redis++", false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}
