#pragma once

#include <optional>

namespace base
{

    /// 存放引用类型的 optional
    template <typename T>
    using ReferenceOptional = std::optional<std::reference_wrapper<T>>;

} // namespace base