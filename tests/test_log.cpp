#include "base/log.hpp"

#include "gtest/gtest.h"

#include <unistd.h>

TEST(base, log)
{
    auto tmp_logger = base::Log{};
    tmp_logger.AddLogFd(STDOUT_FILENO);
    for (size_t i = 0; i < 3; i++)
    {
        tmp_logger.Info("Hello World %s %ld\n", "!!!! ", i);
    }
}