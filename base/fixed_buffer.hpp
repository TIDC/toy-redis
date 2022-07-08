#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <iostream>

namespace base
{

    /// 固定容量的 buffer
    template <size_t Size>
    struct FixedBuffer
    {
        FixedBuffer() = default;
        explicit FixedBuffer(const char *str)
        {
            auto str_length = str != nullptr ? __builtin_strlen(str) : 0;
            assert(str_length <= Size && "FixedBuffer 空间不足");
            FixedBuffer(str, str_length);
        }

        FixedBuffer(const char *str, size_t size)
        {
            assert(size <= Size && "FixedBuffer 空间不足");
            if (size == 0 || str == nullptr)
            {
                return;
            }
            memcpy(data_, str, size);
            used_size_ = size;
        }

        void Append(const char *str)
        {
            auto str_length = str != nullptr ? __builtin_strlen(str) : 0;
            assert(str_length <= FreeSize() && "FixedBuffer 空间不足");
            Append(str, str_length);
        }

        void Append(const char *str, size_t size)
        {
            assert(size <= FreeSize() && "FixedBuffer 空间不足");
            memcpy(data_ + used_size_, str, size);
            used_size_ += size;
        }


        template <size_t OtherSize>
        void Append(const FixedBuffer<OtherSize> &other)
        {
            assert(other.used_size_ <= FreeSize() && "FixedBuffer 空间不足");
            memcpy(data_ + used_size_, other.data_, other.used_size_);
            used_size_ += other.used_size_;
        }

        void Append(const std::string_view &view)
        {
            assert(view.size() <= FreeSize() && "FixedBuffer 空间不足");
            memcpy(data_ + used_size_, view.data(), view.size());
            used_size_ += view.size();
        }

        void Reduce(size_t size)
        {
            assert(size <= used_size_);
            used_size_ -= size;
        }

        void Clear()
        {
            used_size_ = 0;
        }

        /// 获取 buffer 的内存指针
        char *Data()
        {
            return data_;
        }

        /// 刷新已用长度数据
        void Update()
        {
            for (size_t i = 0; i < Capacity(); i++)
            {
                if (data_[i] == '\0')
                {
                    used_size_ = i;
                    break;
                }
            }
            used_size_ = Capacity();
        }

        void Update(int32_t used_size)
        {
            used_size_ = used_size;
        }

        /// 获取 c 语言风格字符串，'\0' 结尾，这个函数会修改 FixedBuffer 的实际内容，
        /// 使用时需要注意是否有影响，如果已经使用的大小和 FixedBuffer 的最大容量相同，'\0' 会
        /// 覆盖最后一个字节。
        const char *CString()
        {
            if (used_size_ < capacity_)
            {
                data_[used_size_] = '\0';
            }
            else
            {
                data_[capacity_ - 1] = '\0';
            }
            return data_;
        }

        size_t UsedSize() const
        {
            return used_size_;
        }

        size_t Capacity() const
        {
            return capacity_;
        }

        size_t FreeSize() const
        {
            assert((capacity_ - used_size_) >= 0);
            return capacity_ - used_size_;
        }

        void Debug() const
        {
            for (size_t i = 0; i < used_size_; i++)
            {
                std::cerr << i << ": " << data_[i] << std::endl;
            }
        }

        char data_[Size]{0};
        size_t used_size_{0};
        const size_t capacity_{Size};
    };

    // FixedBuffer 转 std::string
    template <size_t Size>
    inline std::string ToString(const FixedBuffer<Size> &buffer)
    {
        return std::string{buffer.data_, buffer.used_size_};
    }

    // 判断类型是否为 FixedBuffer 的实例
    template <typename T>
    constexpr inline bool IsFixedBufferHelper = false;

    template <size_t Size>
    constexpr inline bool IsFixedBufferHelper<FixedBuffer<Size>> = true;

    template <typename T>
    constexpr inline bool IsFixedBuffer = IsFixedBufferHelper<std::decay_t<T>>;
} // namespace base