#pragma once

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string_view>

#define HEADER_KEY_LENGTH_SIZE 4
#define HEADER_VALUE_LENGTH_SIZE 4
#define HEADER_HASH_CODE_SIZE 4
#define HEADER_SIZE HEADER_KEY_LENGTH_SIZE + HEADER_VALUE_LENGTH_SIZE + HEADER_HASH_CODE_SIZE

auto hash = std::hash<std::string_view>{};

namespace base
{
    struct ZipMapItem
    {
        std::string_view key;
        std::string_view value;
        uint32_t key_length;
        uint32_t value_length;
        char *pointer = nullptr;
    };

    class ZipMap
    {

    public:
        ZipMap()
        {
            // FIXME: 临时去除未使用警告
            ((void)&max_size_);
        };

        ~ZipMap() = default;

    public:
        void Set(std::string_view key, std::string_view value)
        {
            Set(key.data(), value.data(), key.length(), value.length());
        }

        void Set(char *key, char *value)
        {
            Set(key, value, std::strlen(key), std::strlen(value));
        }

        /// {header:[hash code | key length | value length] | payload: [key | value]}
        bool Set(const char *key, const char *value, uint32_t key_length, uint32_t value_length)
        {
            auto key_view = std::string_view(key);

            if (Exists(key_view))
                return false;

            // 计算插入元素所需长度
            auto required_length = RequiredLength(key_length, value_length);
            auto old_size = size_;
            // 判断是否需要扩容
            Expand(required_length);

            // 直接把新元素放到后面
            auto start_pointer = buffer_.get() + old_size;

            // 对key进行哈希计算
            uint32_t hashcode = hash(key_view);
            auto hashcode_pointer = start_pointer;
            *(uint32_t *)hashcode_pointer = hashcode;
            // 获取存储key的长度的起始位置
            auto key_length_pointer = GetKeyLengthPointer(start_pointer);
            *(uint32_t *)key_length_pointer = key_length;
            // 获取存储value的长度的起始位置
            auto value_length_pointer = GetValueLengthPointer(start_pointer);
            *(uint32_t *)value_length_pointer = value_length;

            // 获取存储key的起始位置
            auto key_pointer = GetKeyPointer(start_pointer);
            std::copy_n(key, key_length, key_pointer);

            // 获取存储value的起始位置
            auto value_pointer = GetValuePointer(start_pointer, key_length);
            std::copy_n(value, value_length, value_pointer);
            used_ += 1;
            return true;
        }

        // 根据 key 获取 value
        std::optional<std::string_view> Get(std::string_view key)
        {

            if (auto item = Find(key))
            {
                return std::optional<std::string_view>{item->value};
            }

            return std::nullopt;
        }

        bool Delete(std::string_view key)
        {
            if (auto item = Find(key))
            {
                uint32_t required_length = RequiredLength(item->key_length, item->value_length);
                return Delete(item->pointer, required_length);
            }
            return false;
        }

        bool Delete(char *start_pointer, uint32_t required_length)
        {
            if (start_pointer != nullptr)
            {
                std::fill_n(start_pointer, HEADER_HASH_CODE_SIZE, '\0');
                *(uint32_t *)(start_pointer + HEADER_HASH_CODE_SIZE) = required_length;
                return true;
            }
            return false;
        }

        void Rewind();
        /// 判断 key 是否存在
        bool Exists(std::string_view key)
        {
            if (used_ == 0)
                return false;

            uint32_t key_hashcode = hash(std::string_view(key));

            uint32_t key_length = 0;
            uint32_t value_length = 0;

            auto start_pointer = buffer_.get();

            while (start_pointer != nullptr)
            {
                auto hashcode_pointer = start_pointer;
                uint32_t hashcode = *(uint32_t *)hashcode_pointer;

                auto key_length_pointer = GetKeyLengthPointer(start_pointer);
                key_length = *(uint32_t *)key_length_pointer;

                auto value_length_pointer = GetValueLengthPointer(start_pointer);
                value_length = *(uint32_t *)value_length_pointer;

                if (key_hashcode != hashcode)
                {
                    start_pointer = Next(start_pointer, key_length, value_length);
                    continue;
                }

                auto key_pointer = GetKeyPointer(start_pointer);
                if (key.compare(std::string_view(key_pointer, key_length)) == 0)
                {
                    return true;
                }
                start_pointer = Next(start_pointer, key_length, value_length);
            }
            return false;
        }

        std::optional<ZipMapItem> Find(std::string_view key)
        {
            if (used_ == 0)
                return std::nullopt;

            // 计算需要查找的 key 的 hashcode
            uint32_t key_hashcode = hash(std::string_view(key));

            uint32_t key_length = 0;
            uint32_t value_length = 0;

            // 头指针
            auto start_pointer = buffer_.get();

            while (start_pointer != nullptr)
            {
                // 获取 hashcode
                auto hashcode_pointer = start_pointer;
                uint32_t hashcode = *(uint32_t *)hashcode_pointer;
                // 获取 key 长度
                auto key_length_pointer = GetKeyLengthPointer(start_pointer);
                key_length = *(uint32_t *)key_length_pointer;
                // 获取 value 长度
                auto value_length_pointer = GetValueLengthPointer(start_pointer);
                value_length = *(uint32_t *)value_length_pointer;

                // 两个 hashcode 对不上，不用走下面了，直接下一个
                if (key_hashcode != hashcode)
                {
                    start_pointer = Next(start_pointer, key_length, value_length);
                    continue;
                }
                // 获取 key 进行匹配
                auto key_pointer = GetKeyPointer(start_pointer);
                auto map_key = std::string_view(key_pointer, key_length);
                if (key.compare(map_key) == 0)
                {
                    // 获取 value 返回回去
                    auto value_pointer = GetValuePointer(start_pointer, key_length);
                    return std::optional<ZipMapItem>{ZipMapItem{
                        key: map_key,
                        value: std::string_view(value_pointer, value_length),
                        key_length: key_length,
                        value_length: value_length,
                        pointer: start_pointer
                    }};
                }
                start_pointer = Next(start_pointer, key_length, value_length);
            }
            return std::nullopt;
        }

        uint8_t Used()
        {
            return used_;
        }

        size_t Size()
        {
            return size_;
        }

    private:
        // 迭代 zip map，根据 key length value length 获得下一个位置的指针
        char *Next(char *data_pointer, size_t key_length, size_t value_length)
        {
            auto map_size = RequiredLength(key_length, value_length);
            auto distance = data_pointer - buffer_.get();
            if (distance + map_size >= size_)
                return nullptr;
            return data_pointer + map_size;
        }
        /// 根据数据块大小偏移指针指向指定的数据
        char *GetKeyLengthPointer(char *start_pointer)
        {
            return start_pointer + HEADER_HASH_CODE_SIZE;
        }
        char *GetValueLengthPointer(char *start_pointer)
        {
            return start_pointer + HEADER_HASH_CODE_SIZE + HEADER_KEY_LENGTH_SIZE;
        }
        char *GetKeyPointer(char *start_pointer)
        {
            return start_pointer + HEADER_SIZE;
        }
        char *GetValuePointer(char *start_pointer, uint32_t key_length)
        {
            return start_pointer + HEADER_SIZE + key_length;
        }
        /// 根据数据块大小偏移指针指向指定的数据 end
        /// 计算出一个数据块所需的大小 多 2 字节隔断
        uint32_t RequiredLength(uint32_t key_length, uint32_t value_length)
        {
            return key_length + value_length + HEADER_SIZE + 2;
        }

        void Expand(size_t size)
        {
            Resize(size += size_);
        }

        void Resize(size_t size)
        {
            auto new_buffer = std::make_unique<char[]>(size);
            std::fill_n(new_buffer.get(), size, '\0');
            if (buffer_ != nullptr)
                std::copy_n(buffer_.get(), size, new_buffer.get());
            buffer_ = std::move(new_buffer);
            size_ = size;
        }

    private:
        std::unique_ptr<char[]> buffer_ = nullptr;
        uint8_t max_size_ = 255;
        uint8_t used_ = 0;
        size_t size_ = 0;
    };

} // namespace base