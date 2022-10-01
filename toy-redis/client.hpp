//
// Created by innoyiya on 2022/7/30.
//
#pragma once
#include "base/redis_database.hpp"
#include "base/simple_dynamic_string.hpp"

namespace tr{
    class RedisClient {
    public:
        RedisClient() = default;
    private:
        int fd;
        base::RedisDB *db;
        base::SimpleDynamicString *queryBuf;
    };
}