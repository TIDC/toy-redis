#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace base
{

    class SimpleDynamicString
    {
    public:
        /// 默认构造空的 SDS
        SimpleDynamicString() = default;

        /// 从指定内存地址构造指定长度的 SDS
        SimpleDynamicString(const char *init, size_t init_length)
            : length_(init_length)
        {
            CreateFrom(init, init_length);
        }

        /// 从 C-Style 字符串构造 SDS
        SimpleDynamicString(const char *init)
        {
            auto init_length = (init == nullptr) ? 0 : std::strlen(init);
            CreateFrom(init, init_length);
        }

        /// 从 string_view 构造 SDS
        SimpleDynamicString(std::string_view init)
        {
            CreateFrom(init.data(), init.length());
        }

        /// 复制构造
        SimpleDynamicString(const SimpleDynamicString &other)
        {
            CopyFrom(other);
        }

        /// 移动构造
        SimpleDynamicString(SimpleDynamicString &&other) noexcept
        {
            Swap(other);
            other.length_ = 0;
            other.free_ = 0;
            other.buffer_ = nullptr;
        }

        /// 复制赋值
        SimpleDynamicString &operator=(const SimpleDynamicString &other)
        {
            CopyFrom(other);
            return *this;
        }

        /// 移动赋值
        SimpleDynamicString &operator=(SimpleDynamicString &&other) noexcept
        {
            Swap(other);
            other.length_ = 0;
            other.free_ = 0;
            other.buffer_ = nullptr;
            return *this;
        }

        /// 获取 SDS 的数据指针
        const char *Data() const
        {
            return buffer_.get();
        }

        /// 获取 SDS 的长度
        size_t Length() const
        {
            return length_;
        }

        /// 剩余未使用的容量
        size_t Avail() const
        {
            return free_;
        }

        /// TODO: 刷新长度数据，重新计算实际字符串的长度
        void UpdateLength();

        /// 追加容量
        void MakeRoomFor(size_t append_length)
        {
            assert(append_length >= 0);

            if (Avail() >= append_length)
            {
                return;
            }

            // 简单两倍扩容
            auto new_length = (Length() + append_length) << 1;
            auto new_buffer = std::make_unique<char[]>(new_length + 1);
            assert(buffer_ != nullptr && "Out Of Memory");
            std::copy_n(buffer_.get(), Length(), new_buffer.get());
            new_buffer[Length()] = '\0';

            std::swap(buffer_, new_buffer);
            free_ = new_length - Length();
        }

        /// 追加内容，从指定内存地址读取指定长度的数据追加到当前 SDS 后面
        void Append(const char *target, size_t length)
        {
            if (Avail() < length)
            {
                MakeRoomFor(length);
            }
            for (int i = 0; i < length; i++)
            {
                buffer_[length_++] = target[i];
            }
            buffer_[length_] = '\0';
        }

        /// 追加内容，追加 C-Style 字符串到当前 SDS 后面
        void Append(const char *target)
        {
            size_t targetLength = strlen(target);
            if (Avail() < targetLength) {
                MakeRoomFor(targetLength);
            }
            for (uint64_t index = 0; target[index] != '\0'; index++) {
                buffer_[length_++] = target[index];
            }
            buffer_[length_] = '\0';
        }

        /// 追加内容，追加 string_view 的内容到当前 SDS 后面
        void Append(std::string_view target);

        /// 追加内容，将另一个 SDS 追加到当前 SDS 后面
        void Append(const SimpleDynamicString &other);

        /// 删除两端的指定字符
        /// TODO: 提供 string_view 版本
        void Trim(const char *c);

        /// 保留指定区间内的内容
        /// TODO: 提供 string_view 版本
        void Range(int64_t start, int64_t end);

        /// 全转小写
        void ToLower();

        /// 全转大写
        void ToUpper();

        /// 分割
        std::vector<std::string_view> Split(std::string_view separator) const;

    private:
        /// 初始化的实现
        void CreateFrom(const char *init, size_t init_length)
        {
            assert(length_ >= 0);

            length_ = init_length;
            free_ = 0;

            if (length_ > 0)
            {
                // 分配的内存比指定字符串多一个字节，存放 '\n'
                buffer_ = std::make_unique<char[]>(length_ + 1);
                assert(buffer_ != nullptr && "Out Of Memory");

                if (init != nullptr)
                {
                    std::copy_n(init, length_, buffer_.get());
                    buffer_[length_] = '\0';
                }
                else
                {
                    std::fill_n(buffer_.get(), length_, '\0');
                }
            }
        }

        /// 复制构造和赋值的实现函数
        void CopyFrom(const SimpleDynamicString &other)
        {
            length_ = other.length_;
            free_ = other.free_;

            if (length_ > 0)
            {
                assert(other.buffer_ != nullptr);

                buffer_ = std::make_unique<char[]>(length_ + 1);
                assert(buffer_ != nullptr && "Out Of Memory");

                std::copy_n(other.buffer_.get(), length_, buffer_.get());
            }
            else
            {
                length_ = 0;
                free_ = 0;
                buffer_ = nullptr;
            }
        }

        /// 交换两个 SDS 的内容
        void Swap(SimpleDynamicString &other) noexcept
        {
            std::swap(length_, other.length_);
            std::swap(free_, other.free_);
            std::swap(buffer_, other.buffer_);
        }

    private:
        uint64_t length_ = 0;
        uint64_t free_ = 0;
        std::unique_ptr<char[]> buffer_ = nullptr;
    };

    /// 比较两个 SDS 的内容是否相同
    inline bool operator==(const SimpleDynamicString &lhs, const SimpleDynamicString &rhs)
    {
        if (lhs.Length() == rhs.Length())
        {
            return std::memcmp(lhs.Data(), rhs.Data(), lhs.Length()) == 0;
        }

        return false;
    }

} // namespace base