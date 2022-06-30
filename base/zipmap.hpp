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
        void Set(const char *key, const char *value, uint32_t key_length, uint32_t value_length)
        {
            auto required_length = RequiredLength(key_length, value_length);
            auto old_size = size_;
            Expand(required_length);

            // 直接把新元素放到后面
            auto start_pointer = buffer_.get() + old_size;

            uint32_t hashcode = hash(std::string_view(key));
            auto hashcode_pointer = start_pointer;
            *(uint32_t *)hashcode_pointer = hashcode;

            auto key_length_pointer = GetKeyLengthPointer(start_pointer);
            *(uint32_t *)key_length_pointer = key_length;

            auto value_length_pointer = GetValueLengthPointer(start_pointer);
            *(uint32_t *)value_length_pointer = value_length;

            auto key_pointer = GetKeyPointer(start_pointer);
            std::copy_n(key, key_length, key_pointer);

            auto value_pointer = GetValuePointer(start_pointer, key_length);
            std::copy_n(value, value_length, value_pointer);
            length_ += 1;
        }

        // 根据 key 获取 value
        std::optional<std::string_view> Get(std::string_view key)
        {
            if (length_ == 0)
                return std::nullopt;

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
                    auto value_pointer = GetValuePointer(start_pointer, key_length);
                    // 都不用动，直接返回 value 指针给 string_view
                    return std::optional<std::string_view>{std::string_view(value_pointer, value_length)};
                }
                start_pointer = Next(start_pointer, key_length, value_length);
            }
            return std::nullopt;
        }

        void Delete();
        void Rewind();

        bool Exists(std::string_view key)
        {
            if (length_ == 0)
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

        uint8_t Length()
        {
            return length_;
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
        size_t RequiredLength(uint32_t key_length, uint32_t value_length)
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
        uint8_t length_ = 0;
        size_t size_ = 0;
        void *tail_pointer_ = nullptr;
    };
} // namespace base