#include "base/message_queue.hpp"
#include "base/time_helper.hpp"

#include <atomic>

#include "gtest/gtest.h"

TEST(base, MessageQueue)
{
    std::atomic_bool stop{false};
    base::MessageQueue<int64_t> que{1024};
    std::thread{[&] {
        for (int64_t i = 0; !stop; i++)
        {
            que.Push(i);
        }
    }}.detach();

    auto begin = base::NowMicroseconds();
    while (true)
    {
        auto result = que.Pop();
        if (result.value() >= 1000000)
        {
            stop = true;
            break;
        }
    }
    auto end = base::NowMicroseconds();

    std::cout << "传输百万条 8 字节长度的消息耗时 " << (end - begin) / 1000 << " ms" << std::endl;
}