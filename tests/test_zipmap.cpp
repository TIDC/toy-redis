#include "base/zipmap.hpp"
#include "gtest/gtest.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

TEST(SDS, ZipMapGet)
{
    base::ZipMap zmap;
    //      0           4           8        12    14    29 -- 31
    // {[hashcode][key_length][value_length][key][value][  ]}
    zmap.Set("hi", "this is hi hah!");
    //      31          35          39       43    14
    // {[hashcode][key_length][value_length][key][value]}
    zmap.Set("hello", "this is hello hah!");

    zmap.Set("王花花", "Hello my name is 王花花!");

    ASSERT_EQ(zmap.Get(std::string_view("hello")) != std::nullopt, true);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}

TEST(SDS, ZipMapExists)
{
    base::ZipMap zmap;
    //      0           4           8        12    14    29 -- 31
    // {[hashcode][key_length][value_length][key][value][  ]}
    zmap.Set("hi", "this is hi hah!");
    //      31          35          39       43    14
    // {[hashcode][key_length][value_length][key][value]}
    zmap.Set("hello", "this is hello hah!");

    zmap.Set("王花花", "Hello my name is 王花花!");

    ASSERT_EQ(zmap.Exists(std::string_view("hello")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("ababa")), false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}

TEST(SDS, ZipMapUsed)
{
    base::ZipMap zmap;
    //      0           4           8        12    14    29 -- 31
    // {[hashcode][key_length][value_length][key][value][  ]}
    zmap.Set("hi", "this is hi hah!");
    //      31          35          39       43    14
    // {[hashcode][key_length][value_length][key][value]}
    zmap.Set("hello", "this is hello hah!");

    zmap.Set("王花花", "Hello my name is 王花花!");

    ASSERT_EQ(zmap.Used() == 3, true);
}

TEST(SDS, ZipMapDelete)
{
    base::ZipMap zmap;
    //      0           4           8        12    14    29 -- 31
    // {[hashcode][key_length][value_length][key][value][  ]}
    zmap.Set("hi", "this is hi hah!");
    //      31          35          39       43    14
    // {[hashcode][key_length][value_length][key][value]}
    zmap.Set("hello", "this is hello hah!");

    zmap.Set("王花花", "Hello my name is 王花花!");

    ASSERT_EQ(zmap.Exists(std::string_view("hello")), true);

    ASSERT_EQ(zmap.Delete(std::string_view("hello")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("hello")), false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}