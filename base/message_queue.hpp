#pragma once

#include "base/time_helper.hpp"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace base
{

    /// 线程安全的消息队列
    /// 参考: https://zhuanlan.zhihu.com/p/525985268
    template <typename T>
    class MessageQueue
    {
        struct MessageNode
        {
            T data;
            MessageNode *next = nullptr;
        };

    public:
        using ValueType = T;

        /// 初始化，max_length 指定队列最大长度
        explicit MessageQueue(size_t max_length)
            : max_massage_count_(max_length)
        {
        }

        /// 启用非阻塞模式。
        /// 当队列为满时，Put() 操作将会无视队列最大容量限制，直接添加新消息。
        /// 当队列为空时，Get() 操作会立即返回一个空的 optional。
        /// 默认为阻塞模式。
        void SetNotWait()
        {
            non_block = true;
            get_cv_.notify_one();
            put_cv_.notify_all();
        }

        /// 新增一个消息
        template <typename Type>
        void Push(Type &&value)
        {
            static_assert(std::is_same_v<std::remove_cvref_t<Type>, T>);
            auto *new_node = new MessageNode{std::forward<Type>(value)};

            std::unique_lock<std::mutex> lock(put_mutex_);
            /// 阻塞模式下等待
            put_cv_.wait(lock, [&] {
                return non_block || message_count_ < max_massage_count_;
            });

            if (put_tail_ == nullptr)
            {
                put_head_ = new_node;
                put_tail_ = new_node;
            }
            else
            {
                put_tail_->next = new_node;
                put_tail_ = new_node;
            }

            message_count_++;
            lock.unlock();
            get_cv_.notify_one();
        }

        /// 获取一个消息
        std::optional<T> Pop()
        {
            std::optional<T> result = std::nullopt;

            std::lock_guard<std::mutex> lock(get_mutex_);
            // 先获取消费者队列的消息，消费者队列清空后再获取生产者队列的消息
            if (get_head_ != nullptr || SwapQueue() > 0)
            {
                result = std::make_optional(std::move(get_head_->data));
                auto *need_delete = get_head_;
                get_head_ = get_head_->next;

                delete need_delete;
            }
            return result;
        }

    private:
        /// 交换生产者队列和消费者队列的内容
        size_t SwapQueue()
        {
            assert(get_mutex_.try_lock() == false && "调用改函数前需要先上消费者锁");

            std::unique_lock<std::mutex> lock(put_mutex_);
            /// 等待有消息加入到生产者队列内
            get_cv_.wait(lock, [&] {
                return non_block || message_count_ > 0;
            });

            get_head_ = put_head_;
            auto result = message_count_;
            put_head_ = nullptr;
            put_tail_ = nullptr;
            message_count_ = 0;

            lock.unlock();
            if (result > max_massage_count_ - 1)
            {
                put_cv_.notify_all();
            }

            return result;
        }

    private:
        const size_t max_massage_count_{0};
        size_t message_count_{0};
        std::atomic_bool non_block{false};
        MessageNode *get_head_{nullptr};
        MessageNode *put_head_{nullptr};
        MessageNode *put_tail_{nullptr};

        std::mutex get_mutex_{};
        std::mutex put_mutex_{};
        std::condition_variable get_cv_{};
        std::condition_variable put_cv_{};
    };

} // namespace base