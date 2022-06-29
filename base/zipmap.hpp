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
    class Zipmap
    {

    public:
        Zipmap()
        {
            // FIXME: 临时去除未使用警告
            ((void)&max_size_);
        };

        ~Zipmap() = default;

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

            std::cout << "required_length" << required_length << "  " << sizeof(buffer_.get()) << std::endl;
            uint32_t hashcode = std::hash<std::string_view>{}(std::string_view(key));

            char *hashcode_pointer = buffer_.get();
            char *key_length_pointer = buffer_.get() + HEADER_HASH_CODE_SIZE;
            char *value_length_pointer = buffer_.get() + HEADER_HASH_CODE_SIZE + HEADER_KEY_LENGTH_SIZE;

            std::copy_n((char *)&hashcode, HEADER_HASH_CODE_SIZE, hashcode_pointer);
            std::copy_n((char *)&key_length, HEADER_KEY_LENGTH_SIZE, key_length_pointer);
            std::copy_n((char *)&value_length, HEADER_VALUE_LENGTH_SIZE, value_length_pointer);

            char *key_pointer = buffer_.get() + HEADER_SIZE;
            // std::cout << "key" << sizeof(key) < < < < std::endl;
            std::copy_n(key, key_length, key_pointer);
            // std::copy_n(value, value_length, buffer_.get() + HEADER_SIZE + key_length);
            std::cout << "required_length" << required_length << "  " << std::strlen(buffer_.get()) << std::endl;
            length_++;
        }

        void Get()
        {
            std::cout << "get zipmap" << std::endl;
            uint32_t hashcode;
            uint32_t key_length;
            uint32_t value_length;
            std::copy_n((uint32_t *)buffer_.get(), HEADER_HASH_CODE_SIZE, &hashcode);
            std::copy_n((uint32_t *)(buffer_.get() + HEADER_HASH_CODE_SIZE), HEADER_KEY_LENGTH_SIZE, &key_length);
            std::copy_n((uint32_t *)(buffer_.get() + HEADER_HASH_CODE_SIZE + HEADER_KEY_LENGTH_SIZE), HEADER_VALUE_LENGTH_SIZE, &value_length);

            auto key = std::make_unique<char[]>(key_length);
            // auto value = std::make_unique<char[]>(value_length);
            std::copy_n(buffer_.get() + HEADER_SIZE, key_length, key.get());
            // std::copy_n(buffer_.get() + HEADER_SIZE + key_length, value_length, value.get());

            std::cout << "hashcode " << hashcode << std::endl;
            std::cout << "key_length " << key_length << std::endl;
            std::cout << "value_length " << value_length << std::endl;
            // FIXME: key 不支持与 std::cout 交互的 operator<<
            // std::cout << "key " << key << std::endl;
            // std::cout << "value " << value << std::endl;
        }

    private:
        size_t RequiredLength(uint32_t key_length, uint32_t value_length)
        {
            return key_length + value_length + HEADER_SIZE + 255;
        }

        void Expand(size_t size)
        {
            if (buffer_ != nullptr)
                size += sizeof(buffer_.get());
            Resize(size);
        }

        void Resize(size_t size)
        {
            auto new_buffer = std::make_unique<char[]>(size + 255);
            if (buffer_ != nullptr)
                std::copy_n(buffer_.get(), size, new_buffer.get());
            buffer_ = std::move(new_buffer);
        }

    private:
        std::unique_ptr<char[]> buffer_ = nullptr;
        uint8_t max_size_ = 255;
        uint8_t length_ = 0;
    };
} // namespace base