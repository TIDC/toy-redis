#pragma once

#include "base/meta_utility.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace base
{

    /// 固定大小的环形队列，在栈上创建时需要确保队列大小不超过实际的栈大小
    template <typename T, size_t QUEUE_SIZE>
    class RingQueue
    {
        class Iterator
        {
        public:
            Iterator(RingQueue &target, size_t index)
                : target_(target), index_(index)
            {
            }

            T &operator*() noexcept
            {
                auto position = (target_.front_ + index_) % target_.Capacity();
                return target_.data_[position];
            }

            const T &operator*() const noexcept
            {
                return target_.data_[index_];
            }

            Iterator &operator++() noexcept
            {
                index_++;
                return *this;
            }

            const Iterator &operator++() const noexcept
            {
                index_++;

                return *this;
            }

            bool operator!=(const Iterator &other) const noexcept
            {
                return index_ != other.index_;
            }

        private:
            RingQueue &target_;
            mutable size_t index_;
        };

    public:
        using value_type = T;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using iterator = Iterator;
        using const_iterator = const Iterator;

        /// 使用 least_uint_t<N> 作为 size_type，可以根据队列最大长度 QUEUE_SIZE 自动选择合
        /// 适的 uintX_t，节省内存的使用
        using size_type = least_uint_t<QUEUE_SIZE>;

        constexpr RingQueue() = default;

        /// 判空
        constexpr bool Empty() const
        {
            return length_ == 0;
        }

        /// 获取元素数量
        constexpr size_type Length() const
        {
            return length_;
        }

        /// 获取队列最大容量
        constexpr size_type Capacity() const
        {
            return QUEUE_SIZE;
        }

        /// 重置
        constexpr void Clear()
        {
            front_ = 0;
            end_ = 0;
            length_ = 0;
        }

        /// 获取队头元素的引用
        constexpr T &Front()
        {
            assert(!Empty());
            return data_[front_];
        }

        constexpr const T &Front() const
        {
            assert(!Empty());
            return data_[front_];
        }

        /// 移除队头元素
        constexpr void Pop()
        {
            assert(!Empty());
            front_ = (front_ + 1) % Capacity();
            length_--;
        }

        /// 入队新元素
        constexpr void Push(const T &value)
        {
            assert(Length() != Capacity());
            data_[end_] = value;
            end_ = (end_ + 1) % Capacity();
            length_++;
        }

        constexpr void Push(T &&value)
        {
            assert(Length() != Capacity());
            data_[end_] = std::move(value);
            end_ = (end_ + 1) % Capacity();
            length_++;
        }

        const_pointer Data() const
        {
            return data_;
        }

        pointer Data()
        {
            return data_;
        }

        constexpr iterator begin() noexcept
        {
            return iterator(*this, 0);
        }

        constexpr iterator end() noexcept
        {
            return iterator(*this, Length());
        }

        constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }

        constexpr const_iterator cend() const noexcept
        {
            return end();
        }

        /// 末尾原地构造新元素
        template <typename... Args>
        auto EmplaceBack(Args &&...args)
            // SFINAE 机制，只有 T 能通过参数 args... 构造时，函数模板才能生效，否则无法编译
            -> std::enable_if_t<
                std::is_same<decltype(T{std::decay_t<Args>{}...}), T>::value, void>
        {
            assert(Length() != Capacity());
            ::new (data_ + end_) T{std::forward<Args>(args)...};
            end_ = (end_ + 1) % Capacity();

            length_++;
        }

    private:
        // 存放元素的数组
        T data_[QUEUE_SIZE]{};
        // 队头下标，闭区间
        size_type front_ = 0;
        // 队尾下标，开区间
        size_type end_ = 0;
        // 有效元素数量
        size_type length_ = 0;
    };

} // namespace base