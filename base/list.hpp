#pragma once

#include <forward_list>
#include <list>
#include <memory>

namespace base
{

    /// std::list 别名
    template <typename T, typename Alloc = std::allocator<T>>
    using List = std::list<T, Alloc>;

    /// std::forward_list 别名
    template <typename T, typename Alloc = std::allocator<T>>
    using ForwardList = std::forward_list<T, Alloc>;

} // namespace base