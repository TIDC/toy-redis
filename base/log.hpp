#pragma once

#include "base/fixed_buffer.hpp"
#include "base/marco.hpp"
#include "base/time_helper.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include <sys/time.h>
#include <unistd.h>

namespace base
{
    namespace
    {
        constexpr static std::array<std::string_view, 5> LEVEL_STRING{
            "trace", "debug", "info", "warn", "error"};
    } // namespace

    /// 日志级别枚举
    enum class LogLevel : uint16_t
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4
    };

    namespace
    {
        /// 日志消息接口类
        struct LogEntryBase
        {
            using WriteFunction = decltype(write);

            /// 通过 writer 输出日志文本，writer 的接口与系统函数 write() 相同
            virtual void LogTo(WriteFunction writer, int32_t fd) const = 0;

            virtual ~LogEntryBase() = default;
        };

        /// 日志消息
        template <typename... Ts>
        class LogEntry : public LogEntryBase
        {
            using ArgsType =
                std::tuple<std::remove_cvref_t<std::decay_t<Ts>>...>;

        public:
            DISABLE_COPY(LogEntry);

            LogEntry(timeval time, LogLevel level, std::string_view fmt,
                     Ts... args)
                : time_(time),
                  level_(level),
                  fmt_(fmt),
                  args_(std::move(args)...)
            {
            }

            LogEntry(LogEntry &&other)
                : time_(other.time_),
                  level_(other.level_),
                  fmt_(other.fmt_),
                  args_(std::move(other.args_))
            {
            }

            LogEntry &operator=(LogEntry &&other)
            {
                time_ = other.time_;
                level_ = other.level_;
                fmt_ = other.fmt_;
                args_ = std::move(other.args_);
            }

            /// 格式化并输出日志
            void LogTo(WriteFunction writer, int32_t fd) const override
            {
                thread_local FixedBuffer<4096> buffer;

                auto str_len = strftime(buffer.Data(), 64, "[%b %d %H:%M:%S",
                                        localtime(&time_.tv_sec));
                buffer.Update(str_len);

                char us[10]{0};
                snprintf(us, 10, ".%ld", time_.tv_usec);
                buffer.Append(us);
                buffer.Append("][");
                buffer.Append(LEVEL_STRING[static_cast<size_t>(level_)]);
                buffer.Append("] ");
                writer(fd, buffer.Data(), buffer.UsedSize());

                std::fill_n(buffer.Data(), buffer.Capacity(), '\0');
                buffer.Clear();

                auto fmt_size = FormatArgs(buffer.Data(), buffer.Capacity());
                assert(fmt_size >= 0);
                // 缓冲区不够大，动态分配更大的缓冲区，直到格式化成功并输出返回
                if (fmt_size >= buffer.Capacity())
                {
                    size_t size = buffer.Capacity();
                    do
                    {
                        size *= 4;
                        auto big_buffer = std::make_unique<char[]>(size);
                        fmt_size = FormatArgs(buffer.Data(), buffer.Capacity());
                        assert(fmt_size >= 0);
                        if (fmt_size < size)
                        {
                            writer(fd, big_buffer.get(), fmt_size);
                            return;
                        }
                    } while (fmt_size >= size);
                }

                writer(fd, buffer.Data(), fmt_size);
            }

        private:
            int32_t FormatArgs(char *buffer, size_t buffer_size) const
            {
                auto snprintf_wrapper = [&](auto &&...args) {
                    return snprintf(buffer, buffer_size, fmt_.data(), args...);
                };
                return std::apply(snprintf_wrapper, args_);
            }

            timeval time_;
            LogLevel level_;
            std::string_view fmt_;
            ArgsType args_;
        };

        template <typename... Ts>
        auto MakeLogEntry(LogLevel level, std::string_view fmt, Ts &&...args)
        {
            timeval time;
            gettimeofday(&time, nullptr);
            return std::make_unique<
                LogEntry<std::remove_cvref_t<std::decay_t<Ts>>...>>(
                time, level, fmt, std::forward<Ts>(args)...);
        }

    } // namespace

    /// 简易异步log
    class Log
    {
        using LoggerFunction = std::function<void(LogEntryBase *)>;

        struct LogTask
        {
            std::unique_ptr<LogEntryBase> entry_;
            std::shared_ptr<LoggerFunction> logger_;
        };

    public:
        static Log &GetInstance()
        {
            static Log ins;
            return ins;
        }

        Log()
        {
            ref_count_++;
            // std::cerr << ref_count_ << std::endl;

            static std::once_flag once;
            std::call_once(once, [&] {
                worker_ = std::thread{WorkerFuntion};
                std::mutex mtx;
                std::unique_lock<std::mutex> lock(mtx);
                notify_run_.wait(lock, [&]() -> bool {
                    return stop_ == false;
                });
                // std::cout << "日志线程启动" << std::endl;
            });

            log_target_ = std::make_shared<std::vector<int32_t>>();
            auto logger = [target = log_target_](LogEntryBase *entry) {
                for (const auto &fd : *target)
                {
                    entry->LogTo(write, fd);
                }
            };
            logger_ =
                std::make_shared<std::function<void(LogEntryBase *)>>(logger);
        }

        ~Log()
        {
            ref_count_--;
            // std::cerr << ref_count_ << std::endl;
            if (ref_count_ == 0)
            {
                // std::cerr << "等待结束" << std::endl;

                stop_ = true;
                // std::cout << "通知2" << std::endl;
                std::unique_lock<std::mutex> lock(queue_mutex_);

                while (!task_queue_.empty())
                {
                    auto &task = task_queue_.front();
                    (*task.logger_)(task.entry_.get());
                    task_queue_.pop();
                }

                notify_.notify_one();
                if (worker_.joinable())
                {
                    worker_.join();
                }
            }
        }

        /// 添加日志输出目标的文件描述符
        void AddLogFd(int32_t fd)
        {
            log_target_->emplace_back(fd);
        }

        template <typename... Ts>
        void Print(LogLevel level, std::string_view fmt, Ts &&...args)
        {
            Post(MakeTask(level, fmt, std::forward<Ts>(args)...));
        }

        template <typename... Ts>
        void Debug(std::string_view fmt, Ts &&...args)
        {
            Print(LogLevel::Debug, fmt, std::forward<Ts>(args)...);
        }

        template <typename... Ts>
        void Info(std::string_view fmt, Ts &&...args)
        {
            Print(LogLevel::Info, fmt, std::forward<Ts>(args)...);
        }

        template <typename... Ts>
        void Warn(std::string_view fmt, Ts &&...args)
        {
            Print(LogLevel::Warn, fmt, std::forward<Ts>(args)...);
        }

        template <typename... Ts>
        void Error(std::string_view fmt, Ts &&...args)
        {
            Print(LogLevel::Error, fmt, std::forward<Ts>(args)...);
        }

    private:
        template <typename... Ts>
        LogTask MakeTask(LogLevel level, std::string_view fmt, Ts &&...args)
        {
            return {MakeLogEntry(level, fmt, std::forward<Ts>(args)...),
                    logger_};
        }

        static void Post(LogTask &&task)
        {
            // std::cerr << "新任务" << std::endl;
            std::unique_lock<std::mutex> lock(queue_mutex_);
            task_queue_.emplace(std::move(task));
            // std::cerr << "taskqueuesize " << task_queue_.size() << std::endl;
            auto now = NowMilliseconds();
            if (now - last_notify_time_ > 1000 ||
                task_queue_.size() >= 10)
            {
                lock.unlock();
                last_notify_time_ = now;
                // std::cout << "通知1" << std::endl;
                notify_.notify_one();
            }
        }

        static void WorkerFuntion()
        {
            stop_ = false;
            notify_run_.notify_one();
            while (!stop_)
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                notify_.wait_for(lock, std::chrono::milliseconds(100));
                if (task_queue_.empty())
                {
                    continue;
                }
                std::queue<LogTask> que;
                task_queue_.swap(que);
                lock.unlock();
                // std::cout << "输出 " << que.size() << std::endl;
                while (!que.empty())
                {
                    auto &task = que.front();
                    (*task.logger_)(task.entry_.get());
                    que.pop();
                }
            }
        }

        static inline std::mutex queue_mutex_;
        static inline std::queue<LogTask> task_queue_;
        static inline std::thread worker_;
        static inline std::condition_variable notify_;
        static inline std::condition_variable notify_run_;
        static inline int64_t last_notify_time_{0};
        static inline std::atomic<bool> stop_{true};
        static inline std::atomic<size_t> ref_count_{0};

        std::shared_ptr<std::vector<int32_t>> log_target_{};
        std::shared_ptr<std::function<void(LogEntryBase *)>> logger_;
    };

    static Log &default_log = Log::GetInstance();

} // namespace base