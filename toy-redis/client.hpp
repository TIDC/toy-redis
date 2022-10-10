//
// Created by innoyiya on 2022/7/30.
//
#pragma once
#include "base/redis_database.hpp"
#include "base/simple_dynamic_string.hpp"
#include "identifier.h"

namespace tr{
    class RedisClient {
    public:
        RedisClient(){
            client_logger = base::Log{};
            client_logger.AddLogFd(STDOUT_FILENO);
        }
        static std::shared_ptr<RedisClient> CreateClient(int fd){
            auto ptr = std::make_shared<RedisClient>();
            ptr.get()->SetFd(fd);
            return ptr;
        }

        int GetFd(){
            return fd;
        }

        void SetFd(int fd) {
            this->fd = fd;
        }

        ErrorCode readQueryFromClient() {
            char buf[net::REDIS_IOBUF_LEN];
            auto lengthOfRead = read(fd, buf, net::REDIS_IOBUF_LEN);
            std::cout << "lengthOfRead is " << lengthOfRead << std::endl;
            if (lengthOfRead == -1) {
                if(errno == EAGAIN){
                    lengthOfRead = 0;
                }else {
                    client_logger.Debug("Reading from client: %s",strerror(errno));
                    return ErrorCode::REDIS_ERR;
                }
            }else if (lengthOfRead == 0) {
                client_logger.Debug("Client closed connection");
                return ErrorCode::REDIS_ERR;
            }else if (lengthOfRead > 0) {
                queryBuf->Append(buf, lengthOfRead);
            }else {
                return ErrorCode::REDIS_ERR;
            }
            processInputBuffer();
            return ErrorCode::REDIS_OK;
        }

        void processInputBuffer() {
            while(queryBuf->Length()) {
                // client 处于某种状态时，立即结束
                if (client_flag::REDIS_BLOCKED & flag_ || client_flag::REDIS_IO_WAIT & flag_) {
                    return;
                }
                // 如果是REDIS_CLOSE_AFTER_REPLY状态，说明链接是在向客户端写入应答后关闭，不能继续增加回复
                if (client_flag::REDIS_CLOSE_AFTER_REPLY & flag_) {
                    return;
                }
                // 如果请求类型是未知的，需要判断是什么类型
                if (request_type_ == RequestType::INITIAL) {
                    if (queryBuf->At(0) == '*') {
                        request_type_ = RequestType::REDIS_REQ_MULTI_BULK;
                    }else {
                        request_type_ = RequestType::REDIS_REQ_INLINE;
                    }
                }

                assert((request_type_ == RequestType::REDIS_REQ_MULTI_BULK || request_type_ == RequestType::REDIS_REQ_INLINE) && "Unknown request type");

                if (request_type_ == RequestType::REDIS_REQ_INLINE) {
                    if (ProcessInlineBuffer() != ErrorCode::REDIS_OK) break;
                }else if (request_type_ == RequestType::REDIS_REQ_MULTI_BULK) {
                    if (ProcessMultiBulkBuffer() != ErrorCode::REDIS_OK) break;
                }

                if (argc == 0) {
                    ResetClient();
                }else {
                    if (ErrorCode::REDIS_OK == ProcessCommand()){
                        ResetClient();
                    }
                }

            }
        }

        ErrorCode ProcessInlineBuffer() {
            return ErrorCode::REDIS_ERR;
        }

        ErrorCode ProcessMultiBulkBuffer() {
            return ErrorCode::REDIS_ERR;
        }

        void ResetClient(){

        }

        ErrorCode ProcessCommand() {

            return ErrorCode::REDIS_ERR;
        }


    private:
        int fd;
        base::RedisDB *db;
        base::SimpleDynamicString *queryBuf;
        base::Log client_logger;
        int flag_{client_flag::INITIAL};
        int request_type_{0};
        int argc{0};

    };
}