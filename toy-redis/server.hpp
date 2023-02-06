#pragma once

#include "base/dictionary.hpp"
#include "base/log.hpp"
#include "base/marco.hpp"
#include "base/simple_dynamic_string.hpp"
#include "net/anet.hpp"
#include "net/default_poller.hpp"
#include "net/io_service.hpp"
#include "net/poller_types.hpp"
#include "toy-redis/identifier.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

namespace tr
{
    using EventHandler =
        std::function<void(int32_t, int32_t, const std::any &)>;
    /// 服务端实现
    class ToyRedisServer
    {
        using IOServiceType = net::IOService<net::DefaultPoller>;
        /// 服务端配置信息
        struct ServerConfig
        {
            int32_t dbNum = 16;               // 最大数据库数量
            const char *bindAddr = "0.0.0.0"; // 绑定地址
            int32_t port = 6758;              // 服务端口
        };

    public:
        DISABLE_COPY_AND_MOVE(ToyRedisServer)

        explicit ToyRedisServer(std::string_view config_file)
        {
            InitConfig();
            LoadServerConfig(config_file);
            // TODO 守护进程初始化
            InitServer();
            // TODO rdb 和 aof 初始化
            auto before_sleep = [&](auto &ios) {
                BeforeSleep(ios);
            };

            if (ipfd_ > 0)
            {
                auto addClient = [&](std::shared_ptr<RedisClient> ptr) {
                    ptr->SetDict(dict_);
                    // ptr->SetOperateEventFunction([&](int32_t fd, net::Event event, EventHandler handler) {
                    //     io_service_.AddEventListener(fd, event, handler)},
                    //                                                                                           [&](int32_t fd, net::Event event) {
                    //     io_service_.DeleteEventListener(fd, event) } });
                    ptr->SetOperateEventFunction(
                        [&](int32_t fd, net::Event event, EventHandler handler) {
                            io_service_.AddEventListener(fd, event, handler);
                        },
                        [&](int32_t fd, net::Event event) {
                            io_service_.DeleteEventListener(fd, event);
                        });
                    list_.emplace_back(ptr);
                    std::cout << "add client" << std::endl;
                    // todo 客户端有读事件时的处理
                    auto clientHandler = [&](auto fd, auto event, const std::any &client_data) {
                        // assert(fd == ptr.get()->GetFd() && "fd不一致");
                        std::vector<std::shared_ptr<RedisClient>>::iterator it;
                        std::shared_ptr<RedisClient> rc;
                        for (it = list_.begin(); it < list_.end(); it++) {
                            if (it->get()->GetFd() == fd) {
                                break;
                            }
                        }
                        assert(it != list_.end() && "找不到客户端");
                        std::cout << "read even have come" << std::endl;
                        auto code = it->get()->readQueryFromClient();
                        if (code == ErrorCode::REDIS_CLOSE) {
                            list_.erase(it);
                        }
                    };
                    auto fd = ptr.get()->GetFd();
                    auto result = io_service_.AddEventListener(fd, net::Read, clientHandler);
                    assert(result && "add even fail");
                };
                auto handle = [&](auto fd, auto event, const std::any &client_data) {
                    netTool_.acceptTcpHandler(fd, addClient);
                };
                io_service_.AddEventListener(ipfd_, net::Read, handle);
            }
            io_service_.SetBeforeSleepCallback(before_sleep);
            io_service_.Run();
        }

        ~ToyRedisServer() = default;

    private:
        /// 初始化配置信息
        void InitConfig()
        {
        }

        /// 加载配置文件
        void LoadServerConfig(std::string_view filename)
        {
        }

        /// 初始化服务器
        void InitServer()
        {
            server_logger_ = base::Log{};
            server_logger_.AddLogFd(STDOUT_FILENO);
            if (config_.port != 0)
            {
                char netErr[net::ANET_ERR_LEN];
                ipfd_ = netTool_.anetTcpServer(netErr, config_.port, config_.bindAddr);
                if (ipfd_ == net::ANET_ERR)
                {
                    server_logger_.Error(std::string_view(netErr));
                    exit(1);
                }
            }
        }

        void BeforeSleep(IOServiceType &io_service)
        {
            // TODO sleep前执行的任务
        }

        void RemoveClient(int fd)
        {
            auto i = 0;
            for (; i < list_.size(); i++)
            {
                if (list_.at(i).get()->GetFd() == fd)
                {
                    break;
                }
            }
            // 删除第 i 个元素，需-1
            list_.erase(list_.begin() + i - 1);
        }

    private:
        ServerConfig config_;
        IOServiceType io_service_;
        net::RedisNet netTool_;
        int ipfd_;
        base::Log server_logger_;
        std::vector<std::shared_ptr<RedisClient>> list_;
        std::shared_ptr<base::Dictionary<std::string, std::string>> dict_{std::make_shared<base::Dictionary<std::string, std::string>>()};
    };

} // namespace tr