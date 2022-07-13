#pragma once

/// 编译时根据当前平台选择最佳的 poller 实现作为默认 poller

#if defined(__linux__)
#include "net/epoll_poller.hpp"
#elif defined(__WIN32__)
#include "net/select_poller.hpp"
#elif defined(__APPLE__)
#include "net/kqueue_poller.hpp.hpp"
#else
#include "net/select_poller.hpp"
#endif

namespace net
{

#if defined(__linux__)
    using DefaultPoller = EpollPoller;
#elif defined(__WIN32__)
    using DefaultPoller = SelectPoller;
#elif defined(__APPLE__)
    using DefaultPoller = KQueuePoller;
#else
    using DefaultPoller = SelectPoller;
#endif

} // namespace net