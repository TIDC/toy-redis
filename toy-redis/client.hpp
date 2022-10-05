//
// Created by innoyiya on 2022/7/30.
//
#pragma once
#include "base/redis_database.hpp"
#include "base/simple_dynamic_string.hpp"
#include "status.h"

namespace tr{
    class RedisClient {
    public:
        RedisClient(){
            client_logger = base::Log{};
            client_logger.AddLogFd(STDOUT_FILENO);
        }
        static std::shared_ptr<RedisClient> CreateClient(int fd){
            return std::shared_ptr<RedisClient>();
        }
        int GetFd(){
            return fd;
        }

        ErrorCode readQueryFromClient() {
            char buf[net::REDIS_IOBUF_LEN];
            auto lengthOfRead = read(fd, buf, net::REDIS_IOBUF_LEN);
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
        }

        void processInputBuffer() {
            //todo 处理buffer命令
        }

    private:
        int fd;
        base::RedisDB *db;
        base::SimpleDynamicString *queryBuf;
        base::Log client_logger;
    };
}