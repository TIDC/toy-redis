#pragma once

#include "concepts/has_stl_value_type.hpp"

#include <memory>
#include <type_traits>

/// 该文件实现了通用迭代器的类型消除容器

namespace tr
{
    namespace
    {

        template <typename T>
        class IteratorInterface
        {
        public:
            virtual ~IteratorInterface() = default;

            /// 获取迭代器指向的元素的引用
            virtual T &Get() = 0;

            /// 下一个元素
            virtual IteratorInterface &Next() = 0;

            /// 前一个元素
            virtual IteratorInterface &Previous() = 0;

            /// 比较与目标迭代器是否相同
            virtual bool Equal(const IteratorInterface &) = 0;
        };

        /// 主模板，适配自定义容器的迭代器
        template <typename T, typename = void>
        class IteratorImplement : public IteratorInterface<typename T::ValueType>
        {
        };

        /// 针对 stl 容器的特化，适配 stl 容器的迭代器
        template <HasStlValueType T>
        class IteratorImplement<T>
            : public IteratorInterface<typename T::value_type>
        {
        public:
            using ValueType = typename T::value_type;

            explicit IteratorImplement(T &target)
                : target_(target)
            {
            }

            ValueType &Get() override
            {
                return *target_;
            }

            IteratorImplement &Next() override
            {
                ++target_;
                return *this;
            }

            IteratorImplement &Previous() override
            {
                --target_;
                return *this;
            }

            bool Equal(const IteratorInterface<ValueType> &other) override
            {
                return dynamic_cast<const IteratorImplement &>(other).target_ ==
                       target_;
            }

        private:
            T target_;
        };

    } // namespace

    template <typename ValueType>
    class Iterator
    {
    public:
        template <typename T>
        explicit Iterator(T iterator)
        {
            using IteratorType = std::remove_cvref_t<T>;
            impl_ = std::make_unique<IteratorImplement<IteratorType>>(iterator);
        }

        ValueType &Get()
        {
            return impl_->Get();
        }

        Iterator &Next()
        {
            impl_->Next();
            return *this;
        }

        Iterator &Previous()
        {
            impl_->Previous();
            return *this;
        }

        bool Equal(Iterator &other)
        {
            return impl_->Equal(*other.impl_);
        }

    private:
        std::unique_ptr<IteratorInterface<ValueType>> impl_;
    };

} // namespace tr