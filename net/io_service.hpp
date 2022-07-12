#pragma once

#include "base/checker.hpp"
#include "base/errno_to_string.hpp"
#include "base/marco.hpp"
#include "base/time_helper.hpp"
#include "ipc/pipe.hpp"
#include "net/concepts/poller.hpp"
#include "net/constants.hpp"
#include "net/default_poller.hpp"
#include "net/poller_types.hpp"
#include "net/timer.hpp"

#include <any>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

#include <unistd.h>
#include <sys/time.h>

namespace net
{

    /// 对应 redis 的 ae.h 和 ae.c
    /// 实现事件循环，提供事件监听和定时任务接口
    ///
    /// IOService 调用 Run() 开始工作后，所有其他成员函数都必须在调用 Run() 的线程上调用，
    /// 禁止跨线程使用。
    template <Poller PollerType = DefaultPoller>
    class IOService
    {
        /// IO 事件处理函数类型，参数 1 是 fd，参数 2 是事件值，参数 3 是注册时填充的数据
        using EventHandler =
            std::function<void(int32_t, int32_t, const std::any &)>;

        /// poller 阻塞前的回调函数类型，接收当前的 IOService 实例的引用作为参数
        using BeforeSleepCallback = std::function<void(IOService &)>;

        /// 自定义任务队列
        using PendingTaskQueue = std::vector<std::function<void(IOService &)>>;

        /// 定时器队列类型，最小堆，按超时时间排序
        using TimerQueue = std::priority_queue<std::shared_ptr<Timer>>;

        struct FileEvent
        {
            int32_t mask_ = Event::None;
            EventHandler handler_;
            std::any client_data_;
        };

    public:
        DISABLE_COPY(IOService);

        // TODO 移动构造和赋值的实现

        IOService() = default;

        void Run()
        {
            assert(!owner_thread_id_.has_value() && "不允许重复调用");
            std::cout << "[ 启动 IOService ] tid=" << std::this_thread::get_id() << std::endl;
            MainLoop();
        }

        /// redis function: aeStop
        void Stop()
        {
            stop_ = true;
            std::cout << "[ 关闭 IOService ] tid=" << std::this_thread::get_id() << std::endl;
        }

        /// 设置 poller 阻塞等待前的回调函数
        void SetBeforeSleepCallback(BeforeSleepCallback functor)
        {
            befer_sleep_callback_ = std::move(functor);
        }

        /// 添加自定义任务到 IOService 内等待下一轮事件循环运行
        void RunInLoop(std::function<void(IOService &)> functor)
        {
            CheckCrossThread();
            pending_task_queue_.emplace_back(std::move(functor));
        }

        /// redis function: aeCreateFileEvent
        /// 新增事件监听
        bool AddEventListener(
            int32_t fd, Event event,
            EventHandler handler, std::any client_data = nullptr)
        {
            CheckCrossThread();
            assert(fd < MAX_NUMBER_OF_FD);

            auto result = poller_.AddEvent(fd, event);
            if (result != 0)
            {
                return false;
            }

            auto &e = events_[fd];
            e.mask_ |= event;
            e.handler_ = std::move(handler);
            e.client_data_ = std::move(client_data);

            return true;
        }

        /// redis function: aeDeleteFileEvent
        /// 移除事件监听
        void DeleteEventListener(int32_t fd, Event event)
        {
            CheckCrossThread();
        }

        /// redis funciton: aeCreateTimeEvent
        /// 新增定时器，返回定时器 id
        size_t SetTimeout(std::function<void()> callback, uint64_t timeout_ms)
        {
            CheckCrossThread();
            auto timer_ptr =
                std::make_shared<Timer>(std::move(callback), timeout_ms);
            assert(timer_ptr);

            auto id = timer_ptr->GetID();
            timer_queue_.emplace(std::move(timer_ptr));
            return id;
        }

        /// 新增周期定时器，返回定时器 id
        size_t SetInterval(std::function<void()> callback, uint64_t timeout_ms)
        {
            CheckCrossThread();
            auto timer_ptr = std::make_shared<Timer>(
                std::move(callback), timeout_ms, true);
            assert(timer_ptr);

            auto id = timer_ptr->GetID();
            timer_queue_.emplace(std::move(timer_ptr));
            return id;
        }

        /// redis function: aeDeleteTimeEvent
        /// 取消定时器，如果定时器还存在，返回 true
        bool CancelTimeout(size_t timer_id)
        {
            CheckCrossThread();
        }

        /// 通知 IOService 立即开始工作
        /// 如果 IOService 内的事件循环正在阻塞等待事件，调用该函数提前结束等待。
        /// 允许跨线程调用
        void WakeUp()
        {
            // TODO 只有在等待时才发送
            // 写入一个字节，触发管道读取端的 io 事件，唤醒 poller
            auto result = write(notify_pipe_.WriteEnd(), "P", 1);
            ASSERT_MSG(result != -1)
                << "写管道错误，errno=" << errno << "，"
                << base::ErrnoToString(errno);
        }

    private:
        /// redis function: aeSearchNearestTimer
        /// 获取最近即将超时的定时器的等待时间，单位毫秒
        /// 如果没有等待的定时器，返回 DEFAULT_TIMEOUT_MS
        /// 如果已经有超时的定时器，返回 0
        int64_t GetNearestTimerTimeout(const timeval &now)
        {
            if (timer_queue_.empty())
            {
                return DEFAULT_TIMEOUT_MS;
            }

            auto next_timer_tv = timer_queue_.top()->GetTimeout();
            auto timeout_tv = base::Subtract(next_timer_tv, now);
            auto timeout_ms = base::ToMilliseconds(timeout_tv);

            return timeout_ms > 0 ? timeout_ms : 0;
        }

        /// redis function: processTimeEvents
        /// 处理超时的定时器，返回被处理的定时器数量
        size_t ProcessTimeoutTimer()
        {
            auto now = base::Now();
            // 如果发现系统时间被回拨，直接清空全部定时器
            auto rollover = CheckClockRollover(now);

            // 系统时间正常，并且没有定时器超时
            if (!rollover && timer_queue_.empty())
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

                // 如果是周期定时器，更新超时时间重新放回 timer_queue_
                if (t->IsCircle())
                {
                    t->UpdateNextCircle();
                    timer_queue_.emplace(std::move(t));
                }
            }

            return count;
        }

        /// redis function: aeProcessEvents
        /// 处理 poller_ 等待到的IO事件，返回被处理的事件数量
        size_t ProcessEvents()
        {
            return 0;
        }

        /// 处理自定义任务队列
        void ProcessPendingTask()
        {
            PendingTaskQueue tasks{};
            tasks.reserve(pending_task_queue_.size());
            tasks.swap(pending_task_queue_);
            for (const auto &task : tasks)
            {
                task(*this);
            }
        }

        /// 处理唤醒通知事件
        void ProcessWakeUpNotify(int32_t fd)
        {
            ASSERT_MSG(fd == notify_pipe_.ReadEnd())
                << "无效参数。通知唤醒的 fd=" << notify_pipe_.ReadEnd()
                << " 参数提供的 fd=" << fd;

            // 如果同时被多条线程唤醒，管道会有多个字节的数据，需要将管道内的全部东西取出
            std::byte byte;
            while (true)
            {
                int result = read(fd, &byte, 1);
                if (result == 0 || result == -1)
                {
                    break;
                }
            }
        }

        /// redis function: aeMain
        /// 事件循环
        /// 处理流程：获取即将超时的定时器时间，设置超时时间给 poller 等待 IO 事件，
        /// 如果有超时或 IO 事件触发，先执行定时器回调，再执行 IO 事件回调，循环往复
        void MainLoop()
        {
            // 绑定当前线程
            owner_thread_id_ = std::this_thread::get_id();
            // 注册用于提前唤醒正在阻塞的 poller 的管道 fd
            notify_pipe_.SetNotWait(ipc::PipePort::ReadEnd, true);
            AddEventListener(
                notify_pipe_.ReadEnd(), Event::Read,
                [&](auto fd, auto, auto) {
                    ProcessWakeUpNotify(fd);
                });

            while (!stop_)
            {
                if (befer_sleep_callback_)
                {
                    befer_sleep_callback_(*this);
                }
                auto now = base::Now();
                auto poller_timout_ms = GetNearestTimerTimeout(now);
                // 如果自定义任务队列有任务在等，让 poller 立即返回
                if (!pending_task_queue_.empty())
                {
                    poller_timout_ms = 0;
                }

                /*auto active_event_size = */ poller_.Poll(poller_timout_ms);

                ProcessTimeoutTimer();
                ProcessEvents();
                ProcessPendingTask();
            }
        }

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
            if (result)
            {
                std::cerr << "WARN: 系统时间回拨" << std::endl;
            }
            return result;
        }

        /// 检测 io_service 启动后，是否被跨线程使用，是就崩溃
        void CheckCrossThread()
        {
            // TODO 仅 DEBUG 模式下才做检查
            // 如果还没有启动，直接返回
            if (!owner_thread_id_.has_value())
            {
                return;
            }
            ASSERT_MSG(owner_thread_id_.value() == std::this_thread::get_id())
                << "IOSvervice 不允许跨线程使用，"
                << "绑定的线程是调用 IOSvervice::Run() 函数的线程。"
                << "绑定的线程 id: " << owner_thread_id_.value() << "，"
                << "当前的线程 id: " << std::this_thread::get_id();
        }

    private:
        /// 默认的 poller 超时时间，单位毫秒
        static constexpr int64_t DEFAULT_TIMEOUT_MS = 1000;

        /// 持有 io_service 的线程的 id
        std::optional<std::thread::id> owner_thread_id_{std::nullopt};
        /// IO 事件监听器
        PollerType poller_{};
        /// 最近一检测系统是否被回拨的时间
        timeval last_rollover_check_time_{};
        /// poller 阻塞等待前的回调函数
        BeforeSleepCallback befer_sleep_callback_;
        /// 注册的 fd 和 事件处理对象
        std::array<FileEvent, MAX_NUMBER_OF_FD> events_{};
        /// 定时器队列
        TimerQueue timer_queue_{};
        /// 自定义任务队列
        PendingTaskQueue pending_task_queue_{};
        /// 用于提前唤醒 poller 的管道
        ipc::SimplePipeline notify_pipe_;
        bool stop_{false};
    };

} // namespace net