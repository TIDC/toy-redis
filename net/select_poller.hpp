#pragma once

#include "base/marco.hpp"
#include "base/ring_queue.hpp"
#include "net/concepts/poller.hpp"
#include "net/constants.hpp"
#include "net/poller_types.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
        int32_t AddEvent(int32_t fd, Event event)
        {
            if (event == Event::read)
            {
                FD_SET(fd, &read_fds_);
            }

            if (event == Event::write)
            {
                FD_SET(fd, &write_fds_);
            }
            return 0;
        }

        /// 移除事件监听
        void DeleteEvent(int32_t fd, Event event)
        {
            if (event == Event::read)
            {
                FD_CLR(fd, &read_fds_);
            }

            if (event == Event::write)
            {
                FD_CLR(fd, &write_fds_);
            }
        }

        int32_t Poll(uint64_t timeout_ms)
        {
            std::memcpy(&read_fds_copy_, &read_fds_, sizeof(fd_set));
            std::memcpy(&write_fds_copy_, &write_fds_, sizeof(fd_set));

            timeval timeout;

            auto result = select(
                max_fd_ + 1,
                &read_fds_copy_,
                &write_fds_copy_,
                nullptr,
                &timeout);

            if (result > 0)
            {
                for (size_t i = 0; i <= max_fd_; i++)
                {
                }
            }
        }

    private:
        int32_t max_fd_{0};
        fd_set read_fds_{};
        fd_set write_fds_{};
        fd_set read_fds_copy_{};
        fd_set write_fds_copy_{};
        
        base::RingQueue<FiredEvent, MAX_NUMBER_OF_FD> fired_fds_;
    };

    static_assert(Poller<SelectPoller>);

} // namespace net