//
// Created by innoyiya on 2022/7/30.
//
#pragma once
#include "base/dictionary.hpp"
#include <any>

namespace base{
    class RedisDB
    {
    private:
        RedisDB() = default;
    public:
        static RedisDB *GetInstance()
        {
            static RedisDB instance;
            return &instance;
        }
    private:
        Dictionary<std::string, std::any> *dict;
    };
}