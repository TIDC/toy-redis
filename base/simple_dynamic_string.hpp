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
        explicit SimpleDynamicString(const char *init)
        {
            auto init_length = (init == nullptr) ? 0 : std::strlen(init);
            CreateFrom(init, init_length);
        }

        /// 从 string_view 构造 SDS
        explicit SimpleDynamicString(std::string_view init)
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
            for (size_t i = 0; i < length; i++)
            {
                buffer_[length_++] = target[i];
                free_--;
            }
            buffer_[length_] = '\0';
        }

        /// 追加内容，追加 C-Style 字符串到当前 SDS 后面
        void Append(const char *target)
        {
            size_t targetLength = strlen(target);
            Append(target, targetLength);
        }

        /// 追加内容，追加 string_view 的内容到当前 SDS 后面
        void Append(std::string_view target)
        {
            Append(target.data());
        }

        /// 追加内容，将另一个 SDS 追加到当前 SDS 后面
        void Append(const SimpleDynamicString &other)
        {
            Append(other.buffer_.get(), other.Length());
        }

        /// 删除两端的指定字符
        /// TODO: 提供 string_view 版本
        void Trim(const char *c)
        {
            size_t lengthOfC = strlen(c);
            assert(length_ > lengthOfC);
            size_t begin = 0;
            for (; begin < lengthOfC; begin++)
            {
                if (c[begin] != buffer_[begin])
                {
                    break;
                }
            }
            size_t end = lengthOfC - 1;
            for (; end >= 0; end--)
            {
                if (c[end] != buffer_[end])
                {
                    break;
                }
            }
            if (begin == lengthOfC && end > lengthOfC)
            {
                Range(begin, Length() - lengthOfC);
                return;
            }
            if (begin == lengthOfC)
            {
                Range(begin, Length());
                return;
            }
            if (end > lengthOfC)
            {
                Range(0, Length() - lengthOfC);
                return;
            }
        }

        void Trim2(const char c)
        {
            auto view = std::string_view(buffer_.get());
            int start = view.find_first_not_of(c);
            int end = view.find_last_not_of(c) + 1;
            Range(start, end);
        }

        /// 保留指定区间内的内容
        /// TODO: 提供 string_view 版本
        void Range(uint64_t start, uint64_t end)
        {
            assert(Length() >= end);
            assert(start >= 0);
            uint64_t freeLength = length_ - end + start;
            if (start != 0)
            {
                uint64_t current = 0;
                for (; start < end; start++)
                {
                    buffer_[current++] = buffer_[start];
                }
                end = current;
            }
            length_ = length_ - freeLength;
            free_ = free_ + freeLength;
            buffer_[end] = '\0';
        }

        /// 全转小写
        void ToLower()
        {
            std::transform(&buffer_[0], &buffer_[Length()], &buffer_[0],
                           [](unsigned char c) { return std::tolower(c); });
        }

        /// 全转大写
        void ToUpper()
        {
            std::transform(&buffer_[0], &buffer_[Length()], &buffer_[0],
                           [](unsigned char c) { return std::toupper(c); });
        }

        /// 分割
        std::vector<std::string_view> Split(std::string_view separator) const
        {
            auto data = std::string_view(buffer_.get());
            std::vector<std::string_view> result;

            if (data.find(separator) == std::string_view::npos)
            {
                result.push_back(data);
                return result;
            }

            int index = 0;
            while (true)
            {
                auto indexOf = data.find(separator, index);
                if (indexOf == std::string_view::npos)
                {
                    result.push_back(data.substr(index));
                    break;
                }
                result.push_back(data.substr(index, indexOf - index));
                index += (indexOf - index) + separator.length();
            }
            return result;
        }

        // 查找目标字符串是否存在，返回下标， 不存在返回 npos
        /// @Abandoned 无用代码
        std::string_view::size_type IndexOf(std::string_view target)
        {
            auto view = std::string_view(buffer_.get());
            return view.find(target);
        }

        // 查找是否包含目标字符串
        /// @Abandoned 无用代码
        bool Contains(std::string_view target)
        {
            return IndexOf(target) != std::string_view::npos;
        }

        char At(int index) {
            // 调用者保证数据不越界
            auto data = buffer_.get();
            return data[index];
        }

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

    namespace literals
    {

        /// 字面量字符串转 SDS
        inline SimpleDynamicString operator""_sds(const char *str, std::size_t len)
        {
            return {str, len};
        }

    } // namespace literals

} // namespace base