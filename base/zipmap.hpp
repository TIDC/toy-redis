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

namespace base
{
    // template <typename VT>
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
            Expand(required_length);

            uint32_t hashcode = std::hash<std::string_view>{}(std::string_view(key));
            char *hashcode_pointer = buffer_.get();
            char *key_length_pointer = buffer_.get() + HEADER_HASH_CODE_SIZE;
            char *value_length_pointer = buffer_.get() + HEADER_HASH_CODE_SIZE + HEADER_KEY_LENGTH_SIZE;
            *hashcode_pointer = hashcode;
            *key_length_pointer = key_length;
            *value_length_pointer = value_length;
            char *key_pointer = buffer_.get() + HEADER_SIZE;
            std::copy_n(key, key_length, key_pointer);
            std::copy_n(value, value_length, buffer_.get() + HEADER_SIZE + key_length);
            length_ += 1;
        }

        std::optional<std::string_view> Get(std::string_view key)
        {

            auto *hashcode_pointer = buffer_.get();
            uint32_t hashcode = *hashcode_pointer;

            auto key_length_pointer = GetKeyLengthPointer(buffer_.get());
            uint32_t key_length = *key_length_pointer;

            auto value_length_pointer = GetValueLengthPointer(buffer_.get());
            uint32_t value_length = *value_length_pointer;

            auto key_pointer = GetKeyPointer(buffer_.get());
            // auto key = std::make_unique<char[]>(key_length);
            // std::copy_n(key_pointer, key_length, key.get());

            auto value_pointer = GetValuePointer(buffer_.get(), key_length);
            // 都不用动，直接返回 value 指针给 string_view
            return std::optional<std::string_view>{std::string_view(value_pointer, value_length)};
            //  : std::nullopt;
            // return std::nullopt;
        }

        void Delete();
        void Rewind();
        void Next();
        void Exists();
        uint8_t Length()
        {
            return length_;
        }

        size_t Size()
        {
            return size_;
        }

    private:
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
        /// 根据数据块大小偏移指针指向指定的数据
        /// 计算出一个数据块所需的大小
        size_t RequiredLength(uint32_t key_length, uint32_t value_length)
        {
            return key_length + value_length + HEADER_SIZE;
        }

        void Expand(size_t size)
        {
            if (buffer_ != nullptr)
                size += sizeof(buffer_.get());
            Resize(size);
        }

        void Resize(size_t size)
        {
            auto new_buffer = std::make_unique<char[]>(size);
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