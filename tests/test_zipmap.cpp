#include "base/zipmap.hpp"
#include "gtest/gtest.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

TEST(SDS, ZipMap)
{
    base::ZipMap zmap;
    //      0           4           8        12    14    29 -- 31
    // {[hashcode][key_length][value_length][key][value][  ]}
    zmap.Set("hi", "this is hi hah!");
    //      31          35          39       43    14
    // {[hashcode][key_length][value_length][key][value]}
    zmap.Set("hello", "this is hello hah!");

    zmap.Set("王花花", "Hello my name is 王花花!");

    if (auto value = zmap.Get(std::string_view("王花花")))
    {
        std::cout << "get 王花花: " << *value << '\n';
    }

    if (auto value = zmap.Get(std::string_view("hello")))
    {
        std::cout << "get hello: " << *value << '\n';
    }

    // FIXME: 字面量字符串不能直接比较
    // ASSERT_EQ("hello" == "Hello Redis++", false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}
