#pragma once

#include "net/concepts/poller.hpp"

/// 对应 redis 的 ae.h 和 ae.c
/// 实现事件循环和提供事件监听接口
template <Poller PollerType>
class IOService
{
};