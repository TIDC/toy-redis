#include "base/time_helper.hpp"
#include "net/io_service.hpp"
#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>
#include <thread>

TEST(io_service, SelectPoller)
{
    net::IOService ios;
    int64_t last_call_say_hello = 0;

    auto say_hello = [&] {
        auto now = base::NowMicroseconds();
        if (last_call_say_hello != 0)
        {
            // 简单场景下定时器误差小于 1 ms
            auto diff = std::abs(300000 - (now - last_call_say_hello));
            std::cout << "周期定时器误差 " << diff << " us" << std::endl;
            EXPECT_LE(diff, 1000);
        }
        std::cout << "hello~~ " << base::NowMilliseconds() << std::endl;
        last_call_say_hello = now;
    };
    auto t1 = ios.SetInterval(say_hello, 300);
    ASSERT_EQ(t1, 0);

    auto quit = [] { exit(0); };
    auto t2 = ios.SetTimeout(quit, 1000);
    ASSERT_EQ(t2, 1);

    std::thread([&] {
        ios.Run();
    }).join();
}