#pragma once

#include <cstddef>

namespace net
{
    namespace
    {

        static constexpr size_t MAX_NUMBER_OF_FD = 10240;
        static constexpr int ANET_ERR_LEN = 256;
        static constexpr int ANET_ERR = -1;
        static constexpr int ANET_OK = 0;
        static constexpr int REDIS_IOBUF_LEN = 1024;

    }
} // namespace net