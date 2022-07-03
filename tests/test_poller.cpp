#include "net/poller_types.hpp"
#include "net/select_poller.hpp"
#include "gtest/gtest.h"
#include <array>
#include <bits/types/struct_timeval.h>
#include <cstdint>
#include <string_view>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

void PipeWrite(int32_t pipe_fd, std::string_view message)
{
    while (true)
    {
        write(pipe_fd, message.data(), message.size());
        sleep(1);
    }
}

TEST(poller, SelectPoller)
{
    net::SelectPoller poller;

    std::array<char, 128> buffer;
    std::array<int32_t, 2> pipeline1;
    std::array<int32_t, 2> pipeline2;

    // 创建两个管道
    ASSERT_EQ(pipe(pipeline1.data()), 0);
    ASSERT_EQ(pipe(pipeline2.data()), 0);

    // 子线程死循环往管道里写消息
    std::thread(PipeWrite, pipeline1[1], "hello").detach();
    std::thread(PipeWrite, pipeline2[1], "world").detach();

    // 主线程监听并获取子线程的消息
    poller.AddEvent(pipeline1[0], net::Event::Read);
    poller.AddEvent(pipeline2[0], net::Event::Read);

    timeval begin;
    timeval now;
    gettimeofday(&begin, nullptr);

    auto consumer = [&](const net::FiredEvent &ev) {
        if (ev.events & net::Event::Read)
        {
            size_t size = read(ev.fd, buffer.data(), buffer.size());
            std::cout
                << "[fd " << ev.fd << " 接收] "
                << std::string_view{buffer.data(), size} << std::endl;
        }
    };

    while (true)
    {
        gettimeofday(&now, nullptr);
        if ((now.tv_sec - begin.tv_sec) >= 4)
        {
            break;
        }

        auto active_fd = poller.Poll(10000);
        if (active_fd > 0)
        {
            poller.ConsumeAll(consumer);
        }
    }
}