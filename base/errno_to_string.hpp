#pragma once

#include <cstring>
#include <string>

namespace base
{

    /// erron 转至字符在，strerror_r() 的封装
    std::string ErrnoToString(int32_t error_code)
    {
        std::string result;
        result.resize(1204);
        strerror_r(errno, result.data(), result.length());
        return result;
    }

} // namespace base