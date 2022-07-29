#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
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

        void Cout()
        {
            std::cout << "key: " << key << " value: " << value
                      << " key_length: " << key_length << " value_length: " << value_length << std::endl;
        }
    };

    struct ZipMapIndex
    {
        uint32_t hashcode;
        uint32_t offset;
    };

    class ZipMap
    {

    public:
        ZipMap() = default;
        ~ZipMap() = default;

    public:
        void Set(std::string_view key, std::string_view value)
        {
            Set(key.data(), value.data(), key.length(), value.length());
        }

        void Set(const char *key, const char *value)
        {
            Set(key, value, std::strlen(key), std::strlen(value));
        }

        /// {header:[hash code | key length | value length] | payload: [key | value]}
        bool Set(const char *key, const char *value, uint32_t key_length, uint32_t value_length)
        {
            auto key_view = std::string_view(key);

            // if (Exists(key_view) || used_ == max_size_)
            //     return false;

            // 计算插入元素所需长度
            auto required_length = RequiredLength(key_length, value_length);
            auto old_size = size_;
            // 判断是否需要扩容
            Expand(required_length);

            // 直接把新元素放到后面
            auto start_pointer = buffer_.get() + old_size;
            // 对key进行哈希计算
            uint32_t hashcode = hash(key_view);
            AddIndex(hashcode, old_size);

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

        // // 根据 key 获取 value
        std::optional<std::string_view> Get(std::string_view key)
        {
            if (auto item = Find(key))
                return std::optional<std::string_view>{item->value};
            return std::nullopt;
        }

        bool Delete(std::string_view key)
        {
            auto item = Find(key);
            if (item && item->pointer != nullptr)
            {
                uint32_t required_length = RequiredLength(item->key_length, item->value_length);
                item->pointer[0] = '\0';
                used_ -= 1;
                wait_freed_ += required_length;
                DelIndex(hash(key), item->pointer);
                return true;
            }
            return false;
        }

        void Rewind();

        /// 压缩空间
        void Zip()
        {
            if (size_ == 0 || wait_freed_ == 0)
                return;

            auto new_size = size_ - wait_freed_;
            if (new_size <= 0)
            {
                buffer_ = nullptr;
                return;
            }
            auto new_buffer = std::make_unique<char[]>(new_size);
            auto new_index = std::make_unique<ZipMapIndex[]>(index_size_);

            auto start_pointer = buffer_.get();
            auto new_start_pointer = new_buffer.get();
            auto new_index_start = uint32_t{0};
            auto new_index_offset = uint32_t{0};
            while (start_pointer != nullptr)
            {
                // 获取 key 长度
                auto key_length_pointer = GetKeyLengthPointer(start_pointer);
                auto key_length = *(uint32_t *)key_length_pointer;
                // 获取 value 长度
                auto value_length_pointer = GetValueLengthPointer(start_pointer);
                auto value_length = *(uint32_t *)value_length_pointer;
                auto required_length = RequiredLength(key_length, value_length);
                if (start_pointer[0] != '\0')
                {
                    std::copy_n(start_pointer, required_length, new_start_pointer);
                    uint32_t hashcode = *(uint32_t *)start_pointer;
                    new_index[new_index_start] = ZipMapIndex{
                        .hashcode = hashcode,
                        .offset = new_index_offset};
                    new_index_start++;
                    new_index_offset += required_length;
                    new_start_pointer += required_length;
                }

                if (static_cast<size_t>((start_pointer - buffer_.get()) + required_length) >= size_)
                    break;

                start_pointer += required_length;
            }
            buffer_ = std::move(new_buffer);
            index_ = std::move(new_index);
            size_ = new_size;
            wait_freed_ = 0;
        }

        /// 判断 key 是否存在
        bool Exists(std::string_view key)
        {
            return Find(key) != std::nullopt;
        }

        std::optional<ZipMapItem> Find(std::string_view key)
        {
            if (used_ == 0)
                return std::nullopt;

            // 计算需要查找的 key 的 hashcode
            uint32_t key_hashcode = hash(std::string_view(key));

            // index 查询流程
            for (int i = 0; i < index_used_; i++)
            {
                if (index_[i].hashcode != 0 && index_[i].hashcode == key_hashcode)
                {
                    auto start_pointer = buffer_.get() + index_[i].offset;
                    auto key_length_pointer = GetKeyLengthPointer(start_pointer);
                    uint32_t key_length = *(uint32_t *)key_length_pointer;

                    auto key_pointer = GetKeyPointer(start_pointer);
                    auto map_key = std::string_view(key_pointer, key_length);
                    if (key.compare(map_key) == 0)
                    {
                        auto value_length_pointer = GetValueLengthPointer(start_pointer);
                        auto value_length = *(uint32_t *)value_length_pointer;

                        auto value_pointer = GetValuePointer(start_pointer, key_length);
                        return std::optional<ZipMapItem>{ZipMapItem{
                            .key = map_key,
                            .value = std::string_view(value_pointer, value_length),
                            .key_length = key_length,
                            .value_length = value_length,
                            .pointer = start_pointer}};
                    }
                }
            }
            return std::nullopt;

            // uint32_t key_length = 0;
            // uint32_t value_length = 0;

            // // 头指针
            // auto start_pointer = buffer_.get();

            // while (start_pointer != nullptr)
            // {
            //     // 获取 hashcode
            //     auto hashcode_pointer = start_pointer;
            //     uint32_t hashcode = *(uint32_t *)hashcode_pointer;
            //     // 获取 key 长度
            //     auto key_length_pointer = GetKeyLengthPointer(start_pointer);
            //     key_length = *(uint32_t *)key_length_pointer;
            //     // 获取 value 长度
            //     auto value_length_pointer = GetValueLengthPointer(start_pointer);
            //     value_length = *(uint32_t *)value_length_pointer;

            //     // 两个 hashcode 对不上，不用走下面了，直接下一个
            //     if (key_hashcode != hashcode)
            //     {
            //         start_pointer = Next(start_pointer, key_length, value_length);
            //         continue;
            //     }
            //     // 获取 key 进行匹配
            //     auto key_pointer = GetKeyPointer(start_pointer);
            //     auto map_key = std::string_view(key_pointer, key_length);
            //     if (key.compare(map_key) == 0)
            //     {
            //         auto value_pointer = GetValuePointer(start_pointer, key_length);
            //         return std::optional<ZipMapItem>{ZipMapItem{
            //             .key = map_key,
            //             .value = std::string_view(value_pointer, value_length),
            //             .key_length = key_length,
            //             .value_length = value_length,
            //             .pointer = start_pointer}};
            //     }
            //     start_pointer = Next(start_pointer, key_length, value_length);
            // }
            // return std::nullopt;
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
            if (static_cast<size_t>(distance + map_size) >= size_)
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
            if (buffer_ != nullptr)
                std::copy_n(buffer_.get(), size, new_buffer.get());
            buffer_ = std::move(new_buffer);
            size_ = size;
        }

        void AddIndex(uint32_t hashcode, uint32_t offset)
        {
            if (index_used_ + 1 >= index_size_)
            {
                index_size_ += 10;
                if (index_size_ >= max_size_)
                    index_size_ = max_size_;
                auto new_index = std::make_unique<ZipMapIndex[]>(index_size_);
                if (index_ != nullptr)
                    std::copy_n(index_.get(), index_used_, new_index.get());
                index_ = std::move(new_index);
            }

            index_[index_used_] = ZipMapIndex{
                .hashcode = hashcode,
                .offset = offset};
            index_used_++;
        }

        void DelIndex(uint32_t hashcode, char *pointer)
        {
            // index 查询流程
            for (int i = 0; i < index_used_; i++)
            {
                if (index_.get()[i].hashcode == hashcode && buffer_.get() + index_.get()[i].offset == pointer)
                {
                    index_.get()[i].hashcode = 0;
                    return;
                }
            }
        }

    private:
        std::unique_ptr<char[]> buffer_ = nullptr;
        std::unique_ptr<ZipMapIndex[]> index_ = nullptr;
        uint8_t max_size_ = 64;
        uint8_t used_ = 0;
        uint8_t index_used_ = 0;
        uint8_t index_size_ = 0;
        size_t size_ = 0;
        size_t wait_freed_ = 0;
    };

} // namespace base