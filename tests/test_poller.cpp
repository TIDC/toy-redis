#include "base/time_helper.hpp"
#include "ipc/pipe.hpp"
#include "net/concepts/poller.hpp"
#include "net/poller_types.hpp"
#include "net/select_poller.hpp"
#include "gtest/gtest.h"
#if !defined(__WIN32__)
#include "net/epoll_poller.hpp"
#endif
#if defined(__APPLE__)
#include "net/kqueue_poller.hpp"
#endif

#include <array>
#include <cstdint>
#include <string_view>
#include <thread>

#include <sys/time.h>
#include <unistd.h>

void PipeWrite(int32_t pipe_fd, std::string_view message)
{
    while (true)
    {
        write(pipe_fd, message.data(), message.size());
        sleep(1);
    }
}

void PollerPipeTest(net::Poller auto &poller)
{
    std::array<char, 128> buffer;
    ipc::SimplePipeline pipeline1;
    ipc::SimplePipeline pipeline2;

    pipeline2.SetNotWait(ipc::PipePort::ReadEnd, true);
    pipeline2.SetNotWait(ipc::PipePort::WriteEnd, true);

    // 子线程死循环往管道里写消息
    std::thread(PipeWrite, pipeline1.WriteEnd(), "hello").detach();
    std::thread(PipeWrite, pipeline2.WriteEnd(), "world").detach();

    // 主线程监听并获取子线程的消息
    poller.AddEvent(pipeline1.ReadEnd(), net::Event::Read);
    poller.AddEvent(pipeline2.ReadEnd(), net::Event::Read);

    auto consumer = [&](const net::FiredEvent &ev) {
        if (ev.events & net::Event::Read)
        {
            size_t size = read(ev.fd, buffer.data(), buffer.size());
            std::cout
                << "[fd " << ev.fd << " 接收] "
                << std::string_view{buffer.data(), size} << std::endl;
        }
    };

    auto begin = base::NowMilliseconds();
    while (true)
    {
        auto now = base::NowMilliseconds();
        if ((now - begin) >= 2000)
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

TEST(poller, SelectPoller)
{
    net::SelectPoller poller;
    PollerPipeTest(poller);
}

#if !defined(__WIN32__)

TEST(poller, EpollPoller)
{
    net::EpollPoller poller;
    PollerPipeTest(poller);
}

#endif

#if defined(__APPLE__)

TEST(poller, EpollPoller)
{
    net::KQueuePoller poller;
    PollerPipeTest(poller);
}

#endif