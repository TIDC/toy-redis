#pragma once

#include <cstdint>

namespace net
{
    // poller 可监听的事件
    enum Event
    {
        None = 0b00,
        Read = 0b01,
        Write = 0b10,
    };
    struct FiredEvent
    {
        int32_t fd;
        int32_t events;
    };
} // namespace net