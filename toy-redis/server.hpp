#pragma once

#include "base/marco.hpp"
#include "base/log.hpp"
#include "net/default_poller.hpp"
#include "net/io_service.hpp"
#include <string_view>
#include "net/anet.hpp"

namespace tr
{

    /// 服务端实现
    class ToyRedisServer
    {
        using IOServiceType = net::IOService<net::DefaultPoller>;
        /// 服务端配置信息
        struct ServerConfig
        {
            int32_t dbNum = 16; // 最大数据库数量
            const char *bindAddr = "0.0.0.0"; // 绑定地址
            int32_t port = 6758;  // 服务端口
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

            if (ipfd > 0)
            {
                auto addClient = [&](std::shared_ptr<RedisClient> ptr) {
                    list.emplace_back(ptr);
                    std::cout << "add client" << std::endl;
                    // todo 客户端有读事件时的处理
                    auto clientHandler = [&](auto fd, auto event, const std::any &client_data){
                        assert(fd == ptr.get()->GetFd() && "fd不一致");
                        ptr.get()->readQueryFromClient();
                    };
                    auto result = io_service_.AddEventListener(ptr.get()->GetFd(), net::Read, clientHandler);
                    assert(result && "add even fail");
                };
                auto handle = [&](auto fd, auto event, const std::any &client_data){
                    netTool.acceptTcpHandler(fd, addClient);
                };
                io_service_.AddEventListener(ipfd, net::Read, handle);
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
            server_logger = base::Log{};
            server_logger.AddLogFd(STDOUT_FILENO);
            if (config_.port != 0)
            {
                char netErr[net::ANET_ERR_LEN];
                ipfd = netTool.anetTcpServer(netErr, config_.port, config_.bindAddr);
                if (ipfd == net::ANET_ERR)
                {
                    server_logger.Error(std::string_view(netErr));
                    exit(1);
                }
            }
        }

        void BeforeSleep(IOServiceType &io_service)
        {
            // TODO sleep前执行的任务

        }

        void RemoveClient(int fd){
            auto i = 0;
            for(; i < list.size(); i++) {
                if (list.at(i).get()->GetFd() == fd) {
                    break;
                }
            }
            // 删除第 i 个元素，需-1
            list.erase(list.begin() + i - 1);
        }

    private:
        ServerConfig config_;
        IOServiceType io_service_;
        net::RedisNet netTool;
        int ipfd;
        base::Log server_logger;
        std::vector<std::shared_ptr<RedisClient>> list;
    };

} // namespace tr