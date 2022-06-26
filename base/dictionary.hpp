#pragma once

#include "base/list.hpp"
#include "base/reference_optional.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <vector>

namespace base
{

    /// @brief 简易哈希表
    /// 哈希冲突用简单的拉链法解决
    /// TODO: 实现后续 redis 版本的渐进式扩容机制
    template <
        typename KeyType = int,
        typename ValueType = int,
        typename HashFunction = std::hash<KeyType>,
        typename EqualFunction = std::equal_to<KeyType>>
    class Dictionary
    {
        static constexpr int64_t HT_INITIAL_EXP = 2;
        static constexpr int64_t HT_INITIAL_SIZE = 1 << HT_INITIAL_EXP;

        static_assert(
            std::is_same_v<decltype(HashFunction()(KeyType{})), size_t>,
            "KeyType 无法被 HashFunction 计算哈希值");
        using Entry = std::pair<KeyType, ValueType>;
        using HashTable = std::vector<ForwardList<Entry>>;

    public:
        Dictionary() = default;

        /// 复制构造
        Dictionary(const Dictionary &other)
        {
            CopyFrom(other);
        }

        /// 移动构造
        Dictionary(Dictionary &&other)
        {
            Swap(other);
            other.table_.clear();
            other.table_.shrink_to_fit();
            other.size_ = 0;
            other.sizemask_ = 0;
            other.used_ = 0;
        }

        /// 复制赋值
        Dictionary &operator=(const Dictionary &other)
        {
            CopyFrom(other);
            return *this;
        }

        /// 移动赋值
        Dictionary &operator=(Dictionary &&other)
        {
            Swap(other);
            other.table_.clear();
            other.table_.shrink_to_fit();
            other.size_ = 0;
            other.sizemask_ = 0;
            other.used_ = 0;
            return *this;
        }

        /// 扩容到指定容量，并重新哈希全部键值对
        bool Expand(size_t size)
        {
            assert(size >= used_ && "不允许比已存储的元素少");

            auto expand_size = NextExpandSize(size);
            assert(expand_size >= size);

            auto new_table = HashTable{};
            auto new_size = expand_size;
            auto new_sizemask = expand_size - 1;
            new_table.resize(expand_size);
            auto new_used = 0;

            for (auto &bucket : table_)
            {
                if (bucket.empty())
                {
                    continue;
                }

                for (auto &entry : bucket)
                {
                    auto key_index = hash_(entry.first) & new_sizemask;
                    new_table[key_index].emplace_front(std::move(entry));
                    new_used++;
                }
            }

            assert(new_used == used_);
            std::swap(table_, new_table);

            return true;
        }

        /// 自适应容量到已经存储的元素的数量，并重新哈希全部键值对
        bool Fit();

        /// 添加新的键值对
        template <typename KT, typename VT>
        bool Add(KT &&key, VT &&value);

        /// 替换字典中键为 key 的值成 value
        template <typename VT>
        bool Replace(const KeyType &key, VT &&value);

        /// 删除键值对
        bool Delete(const KeyType &key);

        /// 查找键值对
        ReferenceOptional<const Entry> Find(const KeyType &key);

        /// 释放全部内存
        void Release();

        /* -- TODO: 迭代器 API --*/

    private:
        /// 拷贝操作的实现
        void CopyFrom(const Dictionary &other)
        {
            table_ = other.table_;
            size_ = other.size_;
            sizemask_ = other.sizemask_;
            used_ = other.used_;
        }

        /// 交换
        void Swap(Dictionary &other)
        {
            std::swap(table_, other.table_);
            std::swap(size_, other.size_);
            std::swap(sizemask_, other.sizemask_);
            std::swap(used_, other.used_);
        }

        /// 计算扩容的目标大小，返回的数值是数列 2^n 中大于 size 的最小值
        int64_t NextExpandSize(uint64_t size)
        {
            uint64_t e = HT_INITIAL_EXP;
            // size 超过 int64_t 能表示的最大值，直接返回 int64_t 的最大值
            if (size >= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            {
                return std::numeric_limits<int64_t>::max();
            }

            while (true)
            {
                if ((uint64_t{1} << e) >= size)
                {
                    return (uint64_t{1} << e);
                }
                e++;
            }
        }

    private:
        HashFunction hash_;
        EqualFunction equal_;
        HashTable table_{};
        size_t size_ = 0;
        size_t sizemask_ = 0;
        size_t used_ = 0;
    };

} // namespace base