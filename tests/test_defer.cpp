#include "base/defer.hpp"

#include "gtest/gtest.h"


TEST(base, defer)
{
    int64_t i = 0;
    {
        i++;
        DEFER
        {
            i++;
        };

        ASSERT_EQ(i, 1);

        DEFER
        {
            i++;
        };
    }

    ASSERT_EQ(i, 3);
}