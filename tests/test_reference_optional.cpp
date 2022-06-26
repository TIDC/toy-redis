#include "base/reference_optional.hpp"
#include "gtest/gtest.h"

size_t g_number;

base::ReferenceOptional<size_t> GlobalRef()
{
    return g_number;
}

base::ReferenceOptional<size_t> EmptyRef()
{
    return std::nullopt;
}

TEST(base, ReferenceOptional)
{
    ASSERT_EQ(EmptyRef().has_value(), false);

    ASSERT_EQ(GlobalRef()->get(), 0);

    auto local = GlobalRef()->get();
    local = 100;
    ASSERT_EQ(local != g_number, true);

    decltype(auto) ref = GlobalRef()->get();
    ref = 100;
    ASSERT_EQ(ref == g_number, true);

    g_number = 200;
    ASSERT_EQ(ref == g_number, true);
}