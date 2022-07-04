#pragma once

#include "base/marco.hpp"
#include "base/time_helper.hpp"
#include "net/timer.hpp"

#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>

#include <sys/time.h>
#include <sys/types.h>

namespace net
{

    /// 定时器类
    ///
    /// 定时器的实际运作依实现事件循环的 IOService 类。
    /// Timer 本身仅仅存储回调函数、延迟时间、超时的绝对时间和是否需要周期调用等信息。
    /// 在事件循环的实现中，需要维护一个定时器集合，每一轮循环开始前，都要从集合找出最近即将超
    /// 时的定时器，计算距离超时的相对时间，传递给 poller 阻塞等待，当 poller 返回后再从集
    /// 合内取出全部超时的定时器并执行里面保存的回调函数。
    /// 如果某个定时器是周期定时器，还需要更新时超时的绝对时间再重新放回定时器集合里。
    class Timer
    {
    public:
        DISABLE_COPY(Timer);

        /// functor 与定时器绑定的回调函数
        /// delay_ms 定时时长，单位毫秒
        /// need_circle 是否为周期定时器，可重复使用
        template <std::invocable Functor>
        Timer(Functor &&functor, uint64_t delay_ms, bool need_circle = false)
            : process_(std::forward<Functor>(functor)),
              id_(LAST_TIMER_ID++),
              delay_(delay_ms),
              need_circle_(need_circle)
        {
            timeout_ = base::Now();
            base::AddMilliseconds(timeout_, delay_);
        }

        Timer(Timer &&other)
            : process_(std::move(other.process_)),
              id_(other.id_),
              delay_(other.delay_),
              timeout_(other.timeout_),
              need_circle_(other.need_circle_)
        {
        }

        Timer &operator=(Timer &&other)
        {
            process_ = std::move(other.process_);
            id_ = other.id_;
            delay_ = other.delay_;
            timeout_ = other.timeout_;
            need_circle_ = other.need_circle_;
            return *this;
        }

        /// 获取定时器 id
        size_t GetID() const
        {
            return id_;
        }

        /// 获取超时的绝对时间
        timeval GetTimeout() const
        {
            return timeout_;
        }

        /// 是否为周期定时器
        bool IsCircle()
        {
            return need_circle_;
        }

        /// 更新周期定时器的超时的绝对时间到下一轮时间
        void UpdateNextCircle()
        {
            assert(need_circle_);
            base::AddMilliseconds(timeout_, delay_);
        }

        /// 执行定时器对象的回调函数
        void operator()() const
        {
            if (process_)
            {
                process_();
            }
        }

        /// 比较大小，比较的是绝对超时时间的大小
        bool operator<(const Timer &other) const
        {
            return base::Less(timeout_, other.timeout_);
        }

        bool operator<=(const Timer &other) const
        {
            return base::Less(timeout_, other.timeout_) ||
                   base::Equal(timeout_, other.timeout_);
        }

        /// 取消定时器
        void Cancel()
        {
            process_ = nullptr;
        }

    private:
        inline static std::atomic<size_t> LAST_TIMER_ID{0};
        std::function<void()> process_;
        size_t id_{};
        /// 延迟时间，单位毫秒
        uint64_t delay_;
        /// 超时的绝对时间，值为 Timer 创建时间加上 delay_
        timeval timeout_{};
        /// 是否为周期定时器
        bool need_circle_{};
    };

    bool operator<(
        const std::shared_ptr<Timer> &t1, const std::shared_ptr<Timer> &t2)
    {
        return *t1 < *t2;
    }

} // namespace net