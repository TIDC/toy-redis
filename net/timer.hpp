#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>

#include <sys/time.h>

namespace net
{

    /// 定时器对象
    class Timer
    {
        static inline std::atomic<size_t> LAST_TIMER_ID{0};

    public:
        template <std::invocable Functor>
        Timer(Functor &&functor, uint64_t delay_ms)
            : process_(std::forward<Functor>(functor)), id_(LAST_TIMER_ID++)
        {
            when_.tv_sec = delay_ms / 1000;
            when_.tv_usec = (delay_ms % 1000) * 1000;
        }

        size_t GetID()
        {
            return id_;
        }

        /// 执行定时器对象的回调函数
        void operator()()
        {
            if (process_)
            {
                process_();
            }
        }

        /// 比较
        bool operator<(const Timer &other)
        {
            if (when_.tv_sec < other.when_.tv_sec)
            {
                return true;
            }
            if (when_.tv_sec == other.when_.tv_sec &&
                when_.tv_usec < other.when_.tv_usec)
            {
                return true;
            }
            return false;
        }

        /// 取消定时器
        void Cancel()
        {
            process_ = nullptr;
        }

    private:
        std::function<void()> process_;
        size_t id_{};
        timeval when_{};
    };

} // namespace net