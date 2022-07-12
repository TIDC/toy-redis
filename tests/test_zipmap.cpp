#include "base/zipmap.hpp"
#include "gtest/gtest.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>
#include <unordered_map>

void AddSomeData(base::ZipMap &zmap)
{
    //      0           4           8         12    14   ---  31
    // {[hashcode][key_length][value_length][key][value]}
    zmap.Set(std::string_view("hi"), std::string_view("this is hi hah!"));
    zmap.Set(std::string_view("王伟"), std::string_view("Hello my name is 王伟!"));
    zmap.Set(std::string_view("王迅"), std::string_view("Hello my name is 王迅!"));
    zmap.Set(std::string_view("李拴蛋"), std::string_view("Hello my name is 李拴蛋!"));
    zmap.Set(std::string_view("狗蛋"), std::string_view("Hello my name is 狗蛋!"));
    zmap.Set(std::string_view("张小花"), std::string_view("Hello my name is 张小花!"));
    zmap.Set(std::string_view("张大花"), std::string_view("Hello my name is 张大花!"));
    zmap.Set(std::string_view("小红花"), std::string_view("Hello my name is 小红花!"));
    zmap.Set(std::string_view("小黄花"), std::string_view("Hello my name is 小黄花!"));
    zmap.Set(std::string_view("小紫花"), std::string_view("Hello my name is 小紫花!"));
    zmap.Set(std::string_view("小小虾"), std::string_view("Hello my name is 小小虾!"));
    zmap.Set(std::string_view("王花花"), std::string_view("Hello my name is 王花花!"));
    for (int i = 0; i < 51; i++)
    {
        std::ostringstream oss;
        oss << "no." << i;
        auto key = oss.str();
        oss << "-"
            << "hello hah!";
        auto value = oss.str();
        zmap.Set(std::string_view(key), std::string_view(value));
    }

    zmap.Set(std::string_view("hello"), std::string_view("this is hello hah!"));
}

TEST(SDS, ZipMapGet)
{
    base::ZipMap zmap;
    AddSomeData(zmap);

    ASSERT_EQ(zmap.Get(std::string_view("hello")) != std::nullopt, true);
    zmap.Find(std::string_view("狗蛋"))->Cout();

    // std::cout << "testSdsAppendSpecifyTheLength pass " << no_n << std::endl;
}

bool temp[10000000];
TEST(SDS, ZipMapExists)
{
    base::ZipMap zmap;
    AddSomeData(zmap);

    for (int i = 0; i < 10000000; i++)
    {
        // zmap.Exists(std::string_view("hello"));
        temp[i] = zmap.Exists(std::string_view("hello"));
    }

    // ASSERT_EQ(zmap.Exists(std::string_view("hello")), true);
    // ASSERT_EQ(zmap.Exists(std::string_view("ababa")), false);
    std::cout << "ZipMapExists " << temp << std::endl;
}

bool temp2[10000000];
TEST(SDS, unordered_map)
{

    std::unordered_map<std::string, std::string> dict;
    dict.insert({"hi", "this is hi hah!"});
    dict.insert({"王伟", "Hello my name is 王伟!"});
    dict.insert({"王迅", "Hello my name is 王迅!"});
    dict.insert({"李拴蛋", "Hello my name is 李拴蛋!"});
    dict.insert({"狗蛋", "Hello my name is 狗蛋!"});
    dict.insert({"张小花", "Hello my name is 张小花!"});
    dict.insert({"张大花", "Hello my name is 张大花!"});
    dict.insert({"小红花", "Hello my name is 小红花!"});
    dict.insert({"小黄花", "Hello my name is 小黄花!"});
    dict.insert({"小紫花", "Hello my name is 小紫花!"});
    dict.insert({"小小虾", "Hello my name is 小小虾!"});
    dict.insert({"王花花", "Hello my name is 王花花!"});
    for (int i = 0; i < 51; i++)
    {
        std::ostringstream oss;
        oss << "no." << i;
        auto key = oss.str();
        oss << "-"
            << "hello hah!";
        auto value = oss.str();
        dict.insert({key, value});
    }

    dict.insert({"hello", "this is hello hah!"});

    for (int i = 0; i < 10000000; i++)
    {
        temp2[i] = dict.contains("hello");
    }

    // ASSERT_EQ(zmap.Exists(std::string_view("hello")), true);
    // ASSERT_EQ(zmap.Exists(std::string_view("ababa")), false);
    std::cout << "unordered_map " << temp2 << std::endl;
}

// TEST(SDS, ZipMapUsed)
// {
//     base::ZipMap zmap;
//     AddSomeData(zmap);

//     ASSERT_EQ(zmap.Used() == 255, true);
// }

TEST(SDS, ZipMapDelete)
{
    base::ZipMap zmap;
    AddSomeData(zmap);

    ASSERT_EQ(zmap.Exists(std::string_view("hello")), true);
    ASSERT_EQ(zmap.Delete(std::string_view("hello")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("hello")), false);
    // std::cout << "testSdsAppendSpecifyTheLength pass " << sds.Data() << std::endl;
}

TEST(SDS, ZipMapZip)
{
    base::ZipMap zmap;
    AddSomeData(zmap);
    ASSERT_EQ(zmap.Exists(std::string_view("hi")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("王花花")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("狗蛋")), true);

    ASSERT_EQ(zmap.Delete(std::string_view("狗蛋")), true);

    auto old_size = zmap.Size();
    zmap.Zip();
    auto now_size = zmap.Size();
    ASSERT_EQ(now_size < old_size, true);
    ASSERT_EQ(zmap.Exists(std::string_view("hi")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("王花花")), true);
    ASSERT_EQ(zmap.Exists(std::string_view("狗蛋")), false);
}