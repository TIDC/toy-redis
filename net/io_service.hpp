#pragma once

#include "base/marco.hpp"
#include "base/time_helper.hpp"
#include "net/concepts/poller.hpp"
#include "net/constants.hpp"
#include "net/timer.hpp"

#include <any>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <queue>
#include <vector>

#include <sys/time.h>

namespace net
{

    /// 对应 redis 的 ae.h 和 ae.c
    /// 实现事件循环和提供事件监听接口
    template <Poller PollerType>
    class IOService
    {
        using EventHandler =
            std::function<void(int32_t, int32_t, const std::any &)>;

        using TimerQueue = std::priority_queue<std::shared_ptr<Timer>>;

        struct FileEvent
        {
            int32_t mask_ = Event::None;
            EventHandler handler_;
            std::any client_data_;
        };

    public:
        DISABLE_COPY(IOService);

        IOService() = default;

        /// redis function: aeStop
        void Stop()
        {
            stop_ = true;
        }

        /// redis function: aeCreateFileEvent
        /// 新增事件监听
        bool AddEventListener(
            int32_t fd, Event event, EventHandler handler, std::any client_data)
        {
            assert(fd < MAX_NUMBER_OF_FD);

            poller_.AddEvent(fd, event);

            auto &e = events_[fd];
            e.mask_ |= event;
            e.handler_ = std::move(handler);
            e.client_data_ = std::move(client_data);
        }

        /// redis function: aeDeleteFileEvent
        /// 移除事件监听
        void DeleteEventListener(int32_t fd, Event event);

        /// redis funciton: aeCreateTimeEvent
        /// 新增定时器，返回定时器 id
        size_t SetTimeout(std::function<void()> callback, uint64_t timeout_ms)
        {
            timer_queue_.emplace(std::move(callback), timeout_ms);
        }

        /// redis function: aeDeleteTimeEvent
        /// 取消定时器，如果定时器还存在，返回 true
        bool CancelTimeout(size_t timer_id);

    private:
        /// redis function: aeSearchNearestTimer
        /// 获取最近即将超时的定时器的超时时间
        /// 如果没有等待的定时器，返回 timeval{INT64_MAX, 0}
        timeval GetNearestTimerTimeout()
        {
            if (timer_queue_.empty())
            {
                return {std::numeric_limits<int64_t>::max(), 0};
            }

            return timer_queue_.top()->GetTimeout();
        }

        /// redis function: processTimeEvents
        /// 处理超时的定时器，返回被处理的定时器数量
        size_t ProcessTimeoutTimer()
        {
            auto now = base::Now();
            // 如果发现系统时间被回拨，直接清空全部定时器
            auto rollover = CheckClockRollover(now);

            // 系统时间正常，并且没有定时器超时
            if (!rollover && base::Less(now, timer_queue_.top()->GetTimeout()))
            {
                return 0;
            }

            std::vector<std::shared_ptr<Timer>> timeout_timers;
            timeout_timers.reserve(rollover ? timer_queue_.size() : 16);

            // 取出超时时间小于等于当前时间的定时器
            // 如果系统时间被回拨就全部取出
            while (!timer_queue_.empty())
            {
                const auto &top_timer = timer_queue_.top();
                if (rollover ||
                    base::Less(top_timer->GetTimeout(), now) ||
                    base::Equal(top_timer->GetTimeout(), now))
                {
                    timeout_timers.emplace_back(timer_queue_.top());
                    timer_queue_.pop();
                }
                else
                {
                    break;
                }
            }

            // 执行定时器的回调函数
            size_t count = 0;
            for (auto &t : timeout_timers)
            {
                (*t)();
                count++;
            }

            return count;
        }

        /// redis function: aeProcessEvents
        /// 处理 poller_ 等待到的IO事件，返回被处理的事件数量
        size_t ProcessEvents()
        {
        }

        /// redis function: aeMain
        /// 事件循环
        /// 处理流程：获取即将超时的定时器时间，设置超时时间给 poller 等待 IO 事件，
        /// 如果有超时或 IO 事件触发，先执行定时器回调，再执行 IO 事件回调，循环往复
        void MainLoop()
        {
            while (!stop_)
            {
                auto now = base::Now();
                auto next_timer_timeout = GetNearestTimerTimeout();
                auto poller_timout = timeval{};
                auto poller_timout_ms = DEFAULT_TIMEOUT;
                if (next_timer_timeout != std::numeric_limits<int64_t>::max())
                {
                    poller_timout = base::Subtract(next_timer_timeout, now);
                    poller_timout_ms = base::ToMilliseconds(poller_timout);
                }

                if (poller_timout_ms < 0)
                {
                    poller_timout_ms = 0;
                }

                auto active_event_size = poller_.Poll(poller_timout_ms);

                ProcessTimeoutTimer();
            }
        }

    private:
        /// 检测系统时间是否回拨
        bool CheckClockRollover(const timeval &now)
        {
            bool result = false;
            /// 现在的时间比上一次检测的时间小，并且小了超过 30 分钟，就认为系统时间被回拨
            if (base::Less(now, last_rollover_check_time_))
            {
                auto diff = base::Subtract(last_rollover_check_time_, now);
                if (base::ToSeconds(diff) > 1800)
                {
                    result = true;
                }
            }
            last_rollover_check_time_ = now;
            return result;
        }

        static constexpr int64_t DEFAULT_TIMEOUT = 1000;

        PollerType poller_{};
        /// 最近一检测系统是否被回拨的时间
        timeval last_rollover_check_time_{};
        /// 注册的 fd 和 事件处理对象
        std::array<FileEvent, MAX_NUMBER_OF_FD> events_{};
        TimerQueue timer_queue_{};
        bool stop_{false};
    };

} // namespace net