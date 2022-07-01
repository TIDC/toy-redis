#pragma once

#include "../poller_types.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>

/** 
    多路 IO 复用器的接口约束
    class PollerName
    {
        PollerName();
        PollerName(const PollerName& other) = delete;
        PollerName& operator=(const PollerName& other) = delete;
        int32_t AddEvent(int32_t fd, net::Event event);
        void DeleteEvent(int32_t fd, net::Event event);
        // Poll() 的返回值为触发的事件数 
        int32_t Poll(uint64_t timeout_ms = 0);
    };
**/
template <typename PollerType>
concept Poller = requires(PollerType poller)
{
    std::is_same_v<decltype(PollerType{}), PollerType>;

    {
        poller.AddEvent(int32_t{}, net::Event{})
        } -> std::same_as<int32_t>;

    poller.DeleteEvent(int32_t{}, net::Event{});

    {
        poller.Poll(uint64_t{})
        } -> std::same_as<int32_t>;
};
