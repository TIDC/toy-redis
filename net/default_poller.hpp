#pragma once

/// 编译时根据当前平台选择最佳的 poller 实现作为默认 poller

#if defined(__linux__)
/// TODO 更换 epoll poller
#include "net/select_poller.hpp"
#elif defined(__WIN32__)
#include "net/select_poller.hpp"
#elif defined(__APPLE__)
/// TODO kqueue poller
#else
#include "net/select_poller.hpp"
#endif

namespace net
{

#if defined(__linux__)
    /// TODO 更换 epoll poller
    using DefaultPoller = SelectPoller;
#elif defined(__WIN32__)
    using DefaultPoller = SelectPoller;
#elif defined(__APPLE__)
    /// TODO kqueue poller
#else
    using DefaultPoller = SelectPoller;
#endif

} // namespace net