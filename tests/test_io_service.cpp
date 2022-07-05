#include "net/default_poller.hpp"
#include "net/io_service.hpp"
#include "gtest/gtest.h"

#include <mutex>
#include <thread>

void SayHallo(net::IOService<net::DefaultPoller> &ios)
{
    std::cout << "hello~~" << std::endl;
}

void StopIOService(net::IOService<net::DefaultPoller> &ios)
{
    std::cout << "关闭自己！！" << std::endl;
    ios.Stop();
}

void IOServiceInit(net::IOService<net::DefaultPoller> &ios)
{
    static std::once_flag once;
    std::call_once(once, [&]() {
        ios.RunInLoop(SayHallo);
        ios.RunInLoop(SayHallo);
        ios.RunInLoop(SayHallo);
        ios.RunInLoop(StopIOService);
    });
}

TEST(io_service, CustomTask)
{
    // 测试 SetBeforeSleepCallback 和 RunInLoop
    net::IOService ios;
    ios.SetBeforeSleepCallback(IOServiceInit);
    std::thread{[&] { ios.Run(); }}.join();
}