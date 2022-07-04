#pragma once

#include "base/dictionary.hpp"
#include "base/marco.hpp"
#include "base/ring_queue.hpp"
#include "net/concepts/poller.hpp"
#include "net/constants.hpp"
#include "net/poller_types.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <sys/select.h>
#include <utility>

namespace net
{

    /// select 封装
    class SelectPoller
    {
    public:
        DISABLE_COPY(SelectPoller);

        SelectPoller()
        {
            FD_ZERO(&read_fds_);
            FD_ZERO(&write_fds_);
        }

        /// 新增事件监听
        int32_t AddEvent(int32_t fd, int32_t events)
        {
            if (fd > max_fd_)
            {
                max_fd_ = fd;
            }
            if (events & Event::Read)
            {
                FD_SET(fd, &read_fds_);
            }

            if (events & Event::Write)
            {
                FD_SET(fd, &write_fds_);
            }
            return 0;
        }

        /// 移除事件监听
        void DeleteEvent(int32_t fd, int32_t events)
        {
            if (events & Event::Read)
            {
                FD_CLR(fd, &read_fds_);
            }

            if (events & Event::Write)
            {
                FD_CLR(fd, &write_fds_);
            }
        }

        int32_t Poll(uint64_t timeout_ms)
        {
            assert(timeout_ms >= 0);
            // 传递给 select() 用的 fd_set 是一次性的，想要安全重用就得复制一份给 select()
            std::memcpy(&read_fds_copy_, &read_fds_, sizeof(fd_set));
            std::memcpy(&write_fds_copy_, &write_fds_, sizeof(fd_set));

            auto timeout = timeval{
                static_cast<int64_t>(timeout_ms / 1000),
                static_cast<int64_t>(timeout_ms % 1000) * 1000};

            auto result = select(
                max_fd_ + 1, &read_fds_copy_, &write_fds_copy_,
                nullptr, &timeout);

            // select() 等到了结果
            size_t evnets_count = 0;
            if (result > 0)
            {
                // 遍历全部 fd，查询记录触发的事件
                for (int32_t i = 0; i <= max_fd_; i++)
                {
                    int32_t events = 0;
                    events |= FD_ISSET(i, &read_fds_copy_);
                    events |= FD_ISSET(i, &write_fds_copy_);

                    if (events != 0)
                    {
                        fired_fds_.EmplaceBack(i, events);
                        evnets_count++;
                    }
                }
            }
            return evnets_count;
        }

        /// 传入一个可调用实例，处理全部触发了读写事件的 fd
        /// 参数 fn 的类型是 void(FiredEvent)
        template <std::invocable<FiredEvent> Consumer>
        void ConsumeAll(Consumer fn)
        {
            while (!fired_fds_.Empty())
            {
                fn(fired_fds_.Front());
                fired_fds_.Pop();
            }
        }

        /// 获取一个触发了读写事件的 fd
        std::optional<FiredEvent> Consume()
        {
            if (fired_fds_.Empty())
            {
                return std::nullopt;
            }

            auto result = std::make_optional(fired_fds_.Front());
            fired_fds_.Pop();
            return result;
        }

    private:
        // 记录最大的 fd 的值，给 select() 用
        int32_t max_fd_{-1};
        fd_set read_fds_{};
        fd_set write_fds_{};
        fd_set read_fds_copy_{};
        fd_set write_fds_copy_{};
        // select() 返回后触发了 io 事件的 fd 队列
        base::RingQueue<FiredEvent, MAX_NUMBER_OF_FD> fired_fds_;
    };

    static_assert(Poller<SelectPoller>);

} // namespace net