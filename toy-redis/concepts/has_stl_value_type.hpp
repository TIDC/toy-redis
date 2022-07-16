#pragma once

namespace tr
{

    /// 检查类型是否拥有公开的成员类型声明 value_type
    template <typename T>
    concept HasStlValueType = requires(T t)
    {
        typename T::value_type;
    };

} // namespace tr