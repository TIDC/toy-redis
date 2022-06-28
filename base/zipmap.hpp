#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string_view>

#define HEADER_BLOCK_SIZE 4
#define HEADER_SIZE 12

namespace base
{
    // template <typename VT>
    class Zipmap
    {

    public:
        Zipmap() = delete;

        Zipmap(std::string_view key, std::string_view value)
        {
            std::uint32_t hashcode = std::hash<std::string_view>{}(key);
            std::uint32_t key_length = key.length() + 1;
            std::uint32_t value_length = value.length() + 1;
            data = std::make_unique<void *>(malloc(HEADER_SIZE + key_length + value_length + 2));

            *(std::uint32_t *)data.get() = hashcode;
            *(std::uint32_t *)(data.get() + HEADER_BLOCK_SIZE) = key_length;
            *(std::uint32_t *)(data.get() + HEADER_BLOCK_SIZE * 2) = value_length;

            // std::copy_n(&hashcode, HEADER_BLOCK_SIZE, (std::uint32_t *)data.get());
            // std::copy_n(&key_length, HEADER_BLOCK_SIZE, (std::uint32_t *)data.get() + HEADER_BLOCK_SIZE);
            // std::copy_n(&value_length, HEADER_BLOCK_SIZE, (std::uint32_t *)data.get() + HEADER_BLOCK_SIZE * 2);

            std::fill_n((char *)data.get() + HEADER_SIZE, key_length + value_length, '\0');

            for (int i = HEADER_SIZE, j = 0; i < key_length + HEADER_SIZE; i++, j++)
            {
                // std::cout << "index " << i << " j " << j << std::endl;
                ((char *)data.get())[i] = key.data()[j];
                // std::cout << "value.data()[j] " << value.data()[j] << " ((char *)data.get())[i] " << ((char *)data.get())[i] << std::endl;
            }

            // std::copy_n(key.data(), key_length, (char *)data.get() + HEADER_SIZE);

            // std::fill_n((char *)data.get() + HEADER_SIZE + key_length, value_length, '\0');
            // std::copy_n(value.data(), value_length, (char *)data.get() + HEADER_SIZE + key_length);

            for (int i = HEADER_SIZE + key_length, j = 0; i < key_length + HEADER_SIZE + value_length; i++, j++)
            {
                // std::cout << "index " << i << " j " << j << std::endl;
                ((char *)data.get())[i] = value.data()[j];
                // std::cout << "value.data()[j] " << value.data()[j] << " ((char *)data.get())[i] " << ((char *)data.get())[i] << std::endl;
            }
        }

        std::uint32_t GetHashCode()
        {
            return *(std::uint32_t *)data.get();
        }

        std::uint32_t GetKeyLength()
        {
            return *(std::uint32_t *)(data.get() + HEADER_BLOCK_SIZE);
        }

        std::uint32_t GetValueLength()
        {
            return *(std::uint32_t *)(data.get() + HEADER_BLOCK_SIZE * 2);
        }

        std::string_view GetKey()
        {
            std::uint32_t key_length = GetKeyLength();
            char key[key_length];
            int index = HEADER_SIZE;
            int indexj = 0;
            while (((char *)data.get())[index] != '\0')
            {
                key[indexj] = ((char *)data.get())[index];
                indexj++;
                index++;
            }
            key[key_length - 1] = '\0';

            std::cout << key << std::endl;

            return std::string_view("hello");
        }

        char *GetValue()
        {
            std::uint32_t key_length = GetKeyLength();
            std::uint32_t value_length = GetValueLength();
            char value[value_length];
            int index = HEADER_SIZE + key_length;
            int indexj = 0;
            while (((char *)data.get())[index] != '\0')
            {
                value[indexj] = ((char *)data.get())[index];
                indexj++;
                index++;
            }
            value[value_length - 1] = '\0';

            return value;
        }

        ~Zipmap() {}

    private:
        std::unique_ptr<void *> data = nullptr;
    };
} // namespace base