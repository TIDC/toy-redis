#pragma once

#include "base/list.hpp"
#include "base/reference_optional.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
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
        typename EqualFunction = std::equal_to<>>
    class Dictionary
    {
        static constexpr int64_t HT_INITIAL_EXP = 2;
        static constexpr int64_t HT_INITIAL_SIZE = 1 << HT_INITIAL_EXP;

        using Entry = std::pair<const KeyType, ValueType>;
        using HashTable = std::vector<ForwardList<Entry>>;

        static_assert(
            std::is_same_v<decltype(HashFunction()(KeyType{})), size_t>,
            "KeyType 无法被 HashFunction 计算哈希值");

    public:
        /// redis function: dictCreate
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
            other.sizeMask_ = 0;
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
            other.sizeMask_ = 0;
            other.used_ = 0;
            return *this;
        }

        /// redis function: dictExpand
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
            assert(new_table.size() == expand_size && "Out of memory");
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
            size_ = new_size;
            sizeMask_ = new_sizemask;

            return true;
        }

        /// redis function: dictResize
        /// 将容量自适应到已存储的元素的数量，并重新哈希全部键值对
        bool Fit();

        /// redis function: dictAdd
        /// 添加新的键值对
        template <typename KT, typename VT>
        bool Add(KT &&key, VT &&value)
        {
            int64_t index = FindBucketIndex(key);
            if (index == -1)
            {
                return false;
            }

            table_[index].push_front(
                std::make_pair(
                    std::forward<KT>(key),
                    std::forward<VT>(value)));

            used_++;

            return true;
        }

        /// redis function: dictReplace
        /// 替换字典中键为 key 的值成 value，如果 key 不在字典里就新增
        /// 返回 true 代表新增键值对，返回 false 代表替换
        template <typename KT, typename VT>
        bool Replace(KT &&key, VT &&value)
        {
            if (Add(std::forward<KT>(key), std::forward<VT>(value)))
            {
                return true;
            }

            auto find_result = Find(key);
            assert(find_result.has_value());

            find_result->get().second = std::forward<VT>(value);

            return false;
        }

        /// redis function: dictReplace
        /// 删除键值对
        bool Delete(const KeyType &key)
        {
            if (Empty()) {
                return false;
            }
            auto index = hash_(key) & sizeMask_;
            auto &bucket = table_[index];
            size_t size = bucket.remove_if([&](Entry entry){return equal_(entry.first, key);});
            if (size > 0) {
                used_--;
                return true;
            }
            return false;
        }

        /// redis function: dictDelete
        /// 查找键值对
        ReferenceOptional<Entry> Find(const KeyType &key)
        {
            if (Empty())
            {
                return std::nullopt;
            }

            auto index = hash_(key) & sizeMask_;
            auto &bucket = table_[index];

            // 查找是否已经有相同的 key
            for (auto &entry : bucket)
            {
                if (equal_(entry.first, key))
                {
                    return entry;
                }
            }

            return std::nullopt;
        }

        /// 获取已经存储的元素数量
        auto ElementSize()
        {
            return used_;
        }

        /// 获取表空位的元素数量
        auto BucketSize()
        {
            return table_.size();
        }

        /// 是否为空
        bool Empty()
        {
            return size_ == 0;
        }

        /// 释放全部内存
        void Release();

        /* -- TODO: 迭代器 API --*/

    private:
        /// 拷贝操作的实现
        void CopyFrom(const Dictionary &other)
        {
            table_ = other.table_;
            size_ = other.size_;
            sizeMask_ = other.sizeMask_;
            used_ = other.used_;
        }

        /// 交换
        void Swap(Dictionary &other)
        {
            std::swap(table_, other.table_);
            std::swap(size_, other.size_);
            std::swap(sizeMask_, other.sizeMask_);
            std::swap(used_, other.used_);
        }

        /// redis function: _dictNextPower
        /// 计算扩容的目标大小，返回的数值是数列 2^n 中大于 size 的最小值
        int64_t NextExpandSize(uint64_t size)
        {
            uint64_t e = HT_INITIAL_EXP;
            // size 超过 int64_t 能表示的最大值，直接返回 int64_t 的最大值
            if (size >= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            {
                return std::numeric_limits<int64_t>::max();
            }

            // 二分查找
            auto idx = 63;
            uint64_t i = 1;
            if (size >= 4294967296) {
                idx -= 32;
                size >>= 32;
            }
            if (size >= 65536) {
                idx -= 16;
                size >>= 16;
            }
            if (size >= 256) {
                idx -= 8;
                size >>= 8;
            }
            if (size >= 16) {
                idx -= 4;
                size >>= 4;
            }
            if (size >= 4) {
                idx -= 2;
                size >>= 2;
            }
            if (size >= 2) {
                idx -= 1;
                size >>= 1;
            }
            return i<<(64-idx);
        }

        /// redis function: _dictExpandIfNeeded
        /// 检查是否需要并进行扩容
        bool ExpandIfNeeded()
        {
            if (size_ == 0)
            {
                return Expand(HT_INITIAL_SIZE);
            }

            if (used_ == size_)
            {
                return Expand(size_ * 2);
            }

            return true;
        }

        /// redis function: _dictKeyIndex
        /// 查找能放下参数提供的 key 的表空位，如果 key 在表里已经存在，返回 -1
        int64_t FindBucketIndex(const KeyType &key)
        {
            // 检查需不需要扩容
            // 扩容失败直接返回 -1。不过目前扩容失败会触发断言错误，这个分支不可能会执行。
            if (!ExpandIfNeeded())
            {
                return -1;
            }

            auto index = hash_(key) & sizeMask_;
            auto &bucket = table_[index];

            // 查找是否已经有相同的 key
            for (const auto &entry : bucket)
            {
                if (equal_(entry.first, key))
                {
                    return -1;
                }
            }

            return index;
        }

    private:
        HashFunction hash_;
        EqualFunction equal_;
        HashTable table_{};
        size_t size_ = 0;
        size_t sizeMask_ = 0;
        size_t used_ = 0;
    };

} // namespace base