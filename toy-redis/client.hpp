//
// Created by innoyiya on 2022/7/30.
//
#pragma once
#include "base/dictionary.hpp"
#include "base/log.hpp"
#include "base/redis_database.hpp"
#include "base/simple_dynamic_string.hpp"
#include "identifier.h"
#include "net/constants.hpp"
#include "net/poller_types.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace tr
{
    using EventHandler =
        std::function<void(int32_t, int32_t, const std::any &)>;
    class RedisClient
    {
    public:
        RedisClient()
        {
            client_logger = base::Log{};
            client_logger.AddLogFd(STDOUT_FILENO);
        }

        RedisClient(const RedisClient &) = delete;
        RedisClient operator=(const RedisClient &) = delete;

        RedisClient(RedisClient &&) = delete;
        RedisClient operator=(RedisClient &&) = delete;

        static std::shared_ptr<RedisClient> CreateClient(int fd)
        {
            auto ptr = std::make_shared<RedisClient>();
            ptr.get()->SetFd(fd);
            return ptr;
        }

        int GetFd()
        {
            return fd_;
        }

        void SetFd(int fd)
        {
            this->fd_ = fd;
        }

        ErrorCode readQueryFromClient()
        {
            char buf[net::REDIS_IOBUF_LEN];
            auto lengthOfRead = read(fd_, buf, net::REDIS_IOBUF_LEN);
            std::cout << "lengthOfRead of fd " << fd_ << " is " << lengthOfRead << std::endl;
            if (lengthOfRead == -1)
            {
                if (errno == EAGAIN)
                {
                    lengthOfRead = 0;
                }
                else
                {
                    client_logger.Debug("Reading from client: %s", strerror(errno));
                    return ErrorCode::REDIS_ERR;
                }
            }
            else if (lengthOfRead == 0)
            {
                client_logger.Debug("Client closed connection");
                return ErrorCode::REDIS_ERR;
            }
            else if (lengthOfRead > 0)
            {
                queryBuf.Append(buf, lengthOfRead);
            }
            else
            {
                return ErrorCode::REDIS_ERR;
            }
            processInputBuffer();
            return ErrorCode::REDIS_OK;
        }

        void processInputBuffer()
        {
            std::cout << "queryBuf is " << queryBuf.Data() << std::endl;
            while (queryBuf.Length())
            {
                // client 处于某种状态时，立即结束
                if (client_flag::REDIS_BLOCKED & flag_ || client_flag::REDIS_IO_WAIT & flag_)
                {
                    return;
                }
                // 如果是REDIS_CLOSE_AFTER_REPLY状态，说明链接是在向客户端写入应答后关闭，不能继续增加回复
                if (client_flag::REDIS_CLOSE_AFTER_REPLY & flag_)
                {
                    return;
                }
                // 如果请求类型是未知的，需要判断是什么类型
                if (request_type_ == RequestType::INITIAL)
                {
                    if (queryBuf.At(0) == '*')
                    {
                        request_type_ = RequestType::REDIS_REQ_MULTI_BULK;
                    }
                    else
                    {
                        request_type_ = RequestType::REDIS_REQ_INLINE;
                    }
                }

                assert((request_type_ == RequestType::REDIS_REQ_MULTI_BULK || request_type_ == RequestType::REDIS_REQ_INLINE) && "Unknown request type");

                if (request_type_ == RequestType::REDIS_REQ_INLINE)
                {
                    if (ProcessInlineBuffer() != ErrorCode::REDIS_OK)
                        break;
                }
                else if (request_type_ == RequestType::REDIS_REQ_MULTI_BULK)
                {
                    if (ProcessMultiBulkBuffer() != ErrorCode::REDIS_OK)
                        break;
                }

                if (argc == 0)
                {
                    ResetClient();
                }
                else
                {
                    if (ErrorCode::REDIS_OK == ProcessCommand())
                    {
                        ResetClient();
                    }
                }
            }
        }

        // 负责解析query中的数据，转化为命令形式存放
        ErrorCode ProcessInlineBuffer()
        {
            auto result = queryBuf.Split(" ");
            argc = result.size();
            argv.swap(result);
            return ErrorCode::REDIS_OK;
        }

        ErrorCode ProcessMultiBulkBuffer()
        {
            return ErrorCode::REDIS_ERR;
        }

        void ResetClient()
        {
        }

        ErrorCode ProcessCommand()
        {
            if (argc >= 2)
            {
                if ("get" == argv[0])
                {
                    client_logger.Info("get key");
                    auto find_result = dict_->Find(argv[1]);
                    if (find_result.has_value())
                    {
                        // fixme: 没有判断查询到的是否有旧数据
                        result_.Append(find_result->get().second);
                        addEvent_(fd_, net::Write, [&](int32_t, int32_t, const std::any &) {
                            SendReplyToClient();
                        });
                    }
                }
                else if ("set" == argv[0] && argc == 3)
                {
                    client_logger.Info("set key");
                    dict_->Add(argv[1], argv[2]);
                }
                return ErrorCode::REDIS_OK;
            }
            return ErrorCode::REDIS_ERR;
        }

        void SetDict(std::shared_ptr<base::Dictionary<std::string, std::string>> dict)
        {
            dict_ = dict;
        }

        void SendReplyToClient()
        {
            if (result_.Length() > 0)
            {
                client_logger.Info("Write data to client");
                auto writeLength = write(fd_, result_.Data(), result_.Length());
                if (writeLength <= 0)
                {
                    client_logger.Debug("write to client: %s", strerror(errno));
                    return;
                }
                result_.Clean();
            }
            if (result_.Length() == 0)
            {
                deleteEvent_(fd_, net::Event::Write);
            }
        }

        void Free()
        {
            deleteEvent_(fd_, net::Event::Read);
            deleteEvent_(fd_, net::Event::Write);
        }

        void SetOperateEventFunction(std::function<void(int32_t, net::Event, EventHandler)> add_fun, std::function<void(int32_t, net::Event)> del_fun)
        {
            addEvent_ = add_fun;
            deleteEvent_ = del_fun;
        }

    private:
        int fd_;
        // base::RedisDB *db_;
        std::shared_ptr<base::Dictionary<std::string, std::string>> dict_;
        base::SimpleDynamicString queryBuf{};
        base::Log client_logger;
        int flag_{client_flag::INITIAL};
        int request_type_{0};
        int argc{0};
        std::vector<std::string> argv;
        base::SimpleDynamicString result_{nullptr};
        std::function<void(int32_t, net::Event)> deleteEvent_;
        std::function<void(int32_t, net::Event, EventHandler)> addEvent_;
    };
} // namespace tr