#pragma once

#include <cstdint>

namespace net
{
    namespace
    {

        // poller 可监听的事件
        enum Event
        {
            None = 0b00,
            Read = 0b01,
            Write = 0b10,
        };

        // poller 监听到事件触发的 fd 和触发的事件
        struct FiredEvent
        {
            int32_t fd;
            int32_t events;
        };

    } // namespace
} // namespace net