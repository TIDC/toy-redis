#pragma once

#include "base/marco.hpp"

#include <concepts>
#include <iostream>
#include <type_traits>
#include <utility>

#define __CREATE_DEFER_NAME(name, count) CAT_SYMBOL(name, count)
#define CREATE_DEFER_NAME __CREATE_DEFER_NAME(__defer_, __COUNTER__)

/// 方便宏
#define DEFER auto CREATE_DEFER_NAME = base::DeferHelper{} << [&]

namespace base
{

    /// 离开作用域后执行一个自定义函数
    template <std::invocable Invocable>
    class Defer
    {
    public:
        DISABLE_COPY(Defer);

        template <std::invocable T>
        explicit Defer(T &&functor)
            : functor_(std::forward<T>(functor)) {}

        ~Defer()
        {
            functor_();
        }

    private:
        Invocable functor_;
    };

    template <std::invocable Invocable>
    auto MakeDefer(Invocable &&functor)
    {
        return Defer<Invocable>{std::forward<Invocable>(functor)};
    }

    /// 辅助重载运算符 operator<< 的空类
    struct DeferHelper
    {
    };

    /// 运算符重载，用于方便宏快速创建 Defer 实例
    template <std::invocable Invocable>
    auto operator<<(DeferHelper, Invocable &&functor)
    {
        return MakeDefer(std::forward<Invocable>(functor));
    }

} // namespace base