//
// Created by innoyiya on 2022/10/5.
//
#pragma once

namespace tr {
    enum ErrorCode{
        REDIS_OK,
        REDIS_ERR,
        REDIS_CLOSE,
    };

    enum RequestType{
        INITIAL,
        REDIS_REQ_INLINE,
        REDIS_REQ_MULTI_BULK
    };

    namespace client_flag {
        static constexpr int INITIAL = 0;
        static constexpr int REDIS_SLAVE = 1 << 0;
        static constexpr int REDIS_MASTER = 1 << 1;
        static constexpr int REDIS_MONITOR = 1 << 2;
        static constexpr int REDIS_MULTI = 1 << 3;
        static constexpr int REDIS_BLOCKED = 1 << 4;
        static constexpr int REDIS_IO_WAIT = 1 << 5;
        static constexpr int REDIS_DIRTY_CAS = 1 << 6;
        // 再向客户端写入完成为置为该状态
        static constexpr int REDIS_CLOSE_AFTER_REPLY = 1 << 7;
        static constexpr int REDIS_UNBLOCKED = 1 << 8;
    }
}
