#pragma once

#include "base/marco.hpp"
#include "concepts/has_stl_value_type.hpp"

#include <memory>
#include <type_traits>
#include <utility>

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

        template <typename T>
        class ConstIteratorInterface
        {
        public:
            virtual ~ConstIteratorInterface() = default;

            /// 获取迭代器指向的元素的引用
            virtual const T &Get() = 0;

            /// 下一个元素
            virtual ConstIteratorInterface &Next() = 0;

            /// 前一个元素
            virtual ConstIteratorInterface &Previous() = 0;

            /// 比较与目标迭代器是否相同
            virtual bool Equal(const ConstIteratorInterface &) = 0;
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

            explicit IteratorImplement(T target)
                : target_(std::move(target))
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
                return ((const IteratorImplement &)(other)).target_ == target_;
            }

        private:
            T target_;
        };

        /// 主模板，适配自定义容器的迭代器
        template <typename T, typename = void>
        class ConstIteratorImplement : public ConstIteratorInterface<typename T::ValueType>
        {
        };

        /// 针对 stl 容器的特化，适配 stl 容器的迭代器
        template <HasStlValueType T>
        class ConstIteratorImplement<T>
            : public ConstIteratorInterface<typename T::value_type>
        {
        public:
            using ValueType = typename T::value_type;

            explicit ConstIteratorImplement(T target)
                : target_(std::move(target))
            {
            }

            const ValueType &Get() override
            {
                return *target_;
            }

            ConstIteratorImplement &Next() override
            {
                ++target_;
                return *this;
            }

            ConstIteratorImplement &Previous() override
            {
                --target_;
                return *this;
            }

            bool Equal(const ConstIteratorInterface<ValueType> &other) override
            {
                return ((const ConstIteratorImplement &)(other)).target_ == target_;
            }

        private:
            T target_;
        };

    } // namespace

    /// 通用迭代器的类型擦除容器
    template <typename ValueType>
    class Iterator
    {
    public:
        DISABLE_COPY(Iterator);

        template <typename T>
        explicit Iterator(T &&iterator)
        {
            using Type = std::remove_reference_t<T>;
            impl_ = std::make_unique<IteratorImplement<Type>>(
                std::forward<T>(iterator));
        }

        Iterator(Iterator &&other)
            : impl_(std::move(other.impl_))
        {
        }

        Iterator *operator=(Iterator &&other)
        {
            impl_ = std::move(other.impl_);
            return *this;
        }

        const ValueType &Get()
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

    /// 通用常量迭代器的类型擦除容器。不可通过迭代器修改引用的元素
    template <typename ValueType>
    class ConstIterator
    {
    public:
        DISABLE_COPY(ConstIterator);

        template <typename T>
        explicit ConstIterator(T &&iterator)
        {
            using Type = std::remove_reference_t<T>;
            impl_ = std::make_unique<ConstIteratorImplement<Type>>(
                std::forward<T>(iterator));
        }

        ConstIterator(ConstIterator &&other)
            : impl_(std::move(other.impl_))
        {
        }

        ConstIterator *operator=(ConstIterator &&other)
        {
            impl_ = std::move(other.impl_);
            return *this;
        }

        const ValueType &Get()
        {
            return impl_->Get();
        }

        ConstIterator &Next()
        {
            impl_->Next();
            return *this;
        }

        ConstIterator &Previous()
        {
            impl_->Previous();
            return *this;
        }

        bool Equal(ConstIterator &other)
        {
            return impl_->Equal(*other.impl_);
        }

    private:
        std::unique_ptr<ConstIteratorInterface<ValueType>> impl_;
    };

} // namespace tr