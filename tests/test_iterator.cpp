#include "toy-redis/iterator.hpp"

#include <list>

#include "gtest/gtest.h"

TEST(toy_redis, Iterator)
{
    std::list<int64_t> list{1, 2, 3, 4, 5};
    tr::Iterator<int64_t> curr{list.begin()};
    tr::Iterator<int64_t> end{list.end()};

    while (!curr.Equal(end))
    {
        std::cout << curr.Get() << std::endl;
        curr.Next();
    }

    std::string hello{"Hello"};
    tr::Iterator<char> curr2{hello.begin()};
    tr::Iterator<char> end2{hello.end()};
    while (!curr2.Equal(end2))
    {
        std::cout << curr2.Get();
        curr2.Next();
    }
    std::cout << std::endl;
}