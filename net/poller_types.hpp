#pragma once

#include <cstdint>

namespace net
{
    namespace
    {

        // poller 可监听的事件
        enum class Event
        {
            read,
            write
        };

        // poller 监听到事件触发的 fd 和触发的事件
        struct FiredEvent
        {
            int32_t fd;
            Event event;
        };

    } // namespace
} // namespace net