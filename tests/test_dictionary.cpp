#include "base/dictionary.hpp"
#include "gtest/gtest.h"

TEST(dict, Create)
{
    base::Dictionary<std::string, int64_t> dict1;
    base::Dictionary<char, int64_t> dict2;
    base::Dictionary<char, std::string> dict3;
    base::Dictionary<std::string, std::string> dict4;
}

TEST(dict, Expand)
{
    base::Dictionary<std::string, int64_t> dict;
    ASSERT_EQ(dict.ElementSize(), 0);
    ASSERT_EQ(dict.Empty(), true);

    dict.Expand(111);
    ASSERT_EQ(dict.BucketSize(), 128);

    dict.Expand(1111);
    ASSERT_EQ(dict.BucketSize(), 2048);
}

TEST(dict, AddAndFind)
{
    base::Dictionary<std::string, int64_t> dict;

    dict.Add("hello", 1024);
    dict.Add("world", 2048);

    ASSERT_EQ(dict.ElementSize(), 2);
    ASSERT_EQ(dict.BucketSize(), 4);

    auto find_hello = dict.Find("hello");
    ASSERT_EQ(find_hello.has_value(), true);
    ASSERT_EQ(find_hello->get().second, 1024);

    auto find_world = dict.Find("world");
    ASSERT_EQ(find_world.has_value(), true);
    ASSERT_EQ(find_world->get().second, 2048);

    find_hello->get().second = 4096;
    find_hello = dict.Find("hello");
    ASSERT_EQ(find_hello.has_value(), true);
    ASSERT_EQ(find_hello->get().second, 4096);


}