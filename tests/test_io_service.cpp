#include "net/default_poller.hpp"
#include "net/io_service.hpp"
#include "net/poller_types.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <unistd.h>

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

TEST(io_service, HandleEvent)
{
    net::IOService ios;
    ipc::SimplePipeline pipe;

    auto handle = [](auto fd, auto ev, auto) {
        char buf[1024]{0};
        read(fd, buf, 1024);
        std::cout << buf << std::endl;
    };
    ios.AddEventListener(pipe.ReadEnd(), net::Event::Read, handle);

    ios.SetTimeout([&] { ios.Stop(); }, 500);

    auto worker1 = std::thread{[&] { ios.Run(); }};
    auto worker2 = std::thread{[&] {
        for (int i = 0; i < 3; i++)
        {
            write(pipe.WriteEnd(), "hello", sizeof("hello"));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }};

    worker2.join();
    worker1.join();
}