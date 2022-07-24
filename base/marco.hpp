#pragma once

/// 禁用复制构造和复制赋值
#define DISABLE_COPY(Class)        \
    Class(const Class &) = delete; \
    Class &operator=(const Class &) = delete;

/// 禁用移动构造和移动赋值
#define DISABLE_MOVE(Class)   \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

/// 禁用复制和移动
#define DISABLE_COPY_AND_MOVE(Class) \
    DISABLE_COPY(Class)              \
    DISABLE_MOVE(Class)

/// 拼接符号
#define CAT_SYMBOL(a, b) a##b
