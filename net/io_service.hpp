#pragma once

#include "base/marco.hpp"
#include "net/concepts/poller.hpp"
#include "net/constants.hpp"
#include "net/timer.hpp"

#include <any>
#include <array>
#include <cassert>
#include <cstdint>
#include <queue>

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
        size_t SetTimeout(std::function<void()> callback, uint64_t timeout_ms);

        /// redis function: aeDeleteTimeEvent
        /// 取消定时器，如果定时器还存在，返回 true
        bool CancelTimeout(size_t timer_id);

    private:
        /// redis function: aeSearchNearestTimer
        /// 获取最近即将超时的定时器的等待时间
        /// 如果没有等待的定时器，返回 timeval{-1, -1}
        timeval GetNearestTimerTimeout();

        /// redis function: processTimeEvents
        /// 处理超时的定时器，返回被处理的定时器数量
        size_t ProcessTimeoutTimer();

        /// redis function: aeProcessEvents
        /// 处理 poller_ 等待到的IO事件，返回被处理的事件数量
        size_t ProcessEvents();

        /// redis function: aeMain
        /// 事件循环
        /// 处理流程：获取即将超时的定时器时间，设置超时时间给 poller 等待 IO 事件，
        /// 如果有超时或 IO 事件触发，先执行定时器回调，再执行 IO 事件回调，循环往复
        void MainLoop();

    private:
        PollerType poller_{};
        /// 注册的 fd 和 事件处理对象
        std::array<FileEvent, MAX_NUMBER_OF_FD> events_{};
        std::priority_queue<Timer> timer_queue_{};
        bool stop_{false};
    };

} // namespace net