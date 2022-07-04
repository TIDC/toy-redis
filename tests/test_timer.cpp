#include "base/time_helper.hpp"
#include "net/io_service.hpp"
#include "gtest/gtest.h"
#include <thread>

TEST(io_service, SelectPoller)
{
    net::IOService ios;

    // ios.SetInterval(
    //     [] {
    //         auto now = base::Now();
    //         std::cout << "hello~~ "
    //                   << now.tv_sec << " "
    //                   << now.tv_usec << std::endl;
    //     },
    //     100);

    // ios.SetTimeout(
    //     [] { exit(0); },
    //     10000);

    // std::thread([&] {
    //     ios.Run();
    // }).join();
}