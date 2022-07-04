#pragma once

#include <cstdint>
#include <sys/time.h>

namespace base
{
    /// 获取当前的时间
    inline timeval Now()
    {
        timeval now;
        gettimeofday(&now, nullptr);
        return now;
    }

    /// timeval 转秒
    inline int64_t ToSeconds(const timeval &t)
    {
        return t.tv_sec;
    }

    /// timeval 转毫秒
    inline int64_t ToMilliseconds(const timeval &t)
    {
        return (t.tv_sec * 1000) + (t.tv_usec / 1000);
    }

    /// timeval 转微秒
    inline int64_t ToMicroseconds(const timeval &t)
    {
        return (t.tv_sec * 1000 * 1000) + t.tv_usec;
    }

    /// timeval 加上毫秒时间
    inline void AddMilliseconds(timeval &tv, int64_t ms)
    {
        auto sec = ms / 1000;
        auto usec = (ms % 1000) * 1000;

        tv.tv_sec += sec;
        tv.tv_usec += usec;

        // 微秒部分超过了 1 秒，进位
        if (tv.tv_usec >= 1000000)
        {
            tv.tv_sec += 1;
            tv.tv_usec %= 1000000;
        }
    }

    inline timeval Subtract(const timeval &t1, const timeval &t2)
    {
        return {t1.tv_sec - t2.tv_sec, t2.tv_usec - t2.tv_usec};
    }

    inline bool Less(const timeval &t1, const timeval &t2)
    {
        if (t1.tv_sec < t2.tv_sec)
        {
            return true;
        }

        if (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec)
        {
            return true;
        }

        return false;
    }

    inline bool Equal(const timeval &t1, const timeval &t2)
    {
        return (t1.tv_sec == t2.tv_sec) && (t1.tv_usec == t2.tv_usec);
    }
} // namespace base