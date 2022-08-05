#pragma once

#include "toy-redis/concepts/has_stl_value_type.hpp"
#include "toy-redis/iterator.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace tr
{
    namespace
    {

        template <typename T>
        class ListInterface
        {
        public:
            virtual ~ListInterface() = default;
            /*=== 修改类接口 ===*/
            /// 表头插入新元素
            virtual void PushFront(const T &value) = 0;
            virtual void PushFront(T &&value) = 0;
            /// 表尾插入新元素
            virtual void PushBack(const T &value) = 0;
            virtual void PushBack(T &&value) = 0;
            /// 移除表头元素
            virtual void PopFront() = 0;
            /// 移除表尾元素
            virtual void PopBack() = 0;
            /// 在指定元素的位置前或后插入新元素
            virtual void Insert(const T &target, const T &value, bool after) = 0;
            virtual void Insert(const T &target, T &&value, bool after) = 0;
            /// 删除指定元素
            virtual void Delete(const T &value) = 0;

            /*=== 访问类接口 ===*/
            /// 获取列表元素数量
            virtual size_t Size() = 0;
            /// 获取头部元素的引用
            virtual T &Front() = 0;
            /// 获取尾部元素的引用
            virtual T &Back() = 0;
            /// 获取特定位置元素的
            virtual T &Index(size_t index) = 0;
            /// 获取从 index 位置开始的正向迭代器
            virtual Iterator<T> Iteratro(size_t index) = 0;
        };

        template <typename T, typename = void>
        class ListImplement : public ListInterface<typename T::ValueType>
        {
        };

        template <HasStlValueType T>
        class ListImplement<T> : public ListInterface<typename T::value_type>
        {
        public:
            /// 从一个 List 类型复制构造
            explicit ListImplement(const T &target)
            {
                target_ = target;
            }

            /// 获取一个 List 类型移动构造
            explicit ListImplement(T &&target)
            {
                target_ = std::move(target);
            }

            /*=== 修改类接口 ===*/
            /// 表头插入新元素
            void PushFront(const T &value) override
            {
                target_.push_front(value);
            }

            void PushFront(T &&value) override
            {
                target_.push_front(std::move(value));
            }

            /// 表尾插入新元素
            void PushBack(const T &value) override
            {
                target_.push_back(value);
            }

            void PushBack(T &&value) override
            {
                target_.push_back(std::move(value));
            }

            /// 移除表头元素
            void PopFront() override
            {
                target_.pop_front();
            }

            /// 移除表尾元素
            void PopBack() override
            {
                target_.pop_back();
            }

            /// 在指定位置前或后插入新元素
            /// TODO 性能较差，需要想办法实现通过通用迭代器直接插入元素
            void Insert(size_t index, const T &value, bool after) override
            {
                assert(index >= 0);
                assert(index <= Size());
                auto iter = target_.begin();
                for (size_t i = 0; i < index; i++)
                {
                    iter++;
                }
                if (after)
                {
                    iter++;
                }

                target_.insert(iter, value);
            }

            void Insert(size_t index, T &&value, bool after) override
            {
                assert(index >= 0);
                assert(index <= Size());
                auto iter = target_.begin();
                for (size_t i = 0; i < index; i++)
                {
                    iter++;
                }
                if (after)
                {
                    iter++;
                }

                target_.insert(iter, std::move(value));
            }

            /// 删除指定元素
            void Delete(const T &value) override
            {
                target_.remove(value);
            }

            /*=== 访问类接口 ===*/
            /// 获取列表元素数量
            size_t Size() override
            {
                return target_.size();
            }

            /// 获取头部元素的引用
            T &Front() override
            {
                return target_.front();
            }

            /// 获取尾部元素的引用
            T &Back() override
            {
                return target_.back();
            }

            /// 获取特定位置元素的
            T &Index(size_t index) override
            {
                assert(false && "未实现");
            }

            /// 获取从 index 位置开始的正向迭代器
            Iterator<T> Iteratro(size_t in) override
            {
                assert(false && "未实现");
            }

        private:
            T target_;
        };

    } // namespace

    /// 双端链表的类型擦除容器
    template <typename ValueType>
    class List
    {
    public:
        template <typename ListType>
        List(ListType &&target)
        {
            using Type = std::remove_cvref_t<ListType>;
            static_assert(std::is_same_v<ValueType, typename Type::value_type>);
            target_ = std::make_unique<ListImplement<Type>>(
                std::forward<ListType>(target));
        }

    private:
        std::unique_ptr<ListInterface<ValueType>> target_;
    };

} // namespace tr