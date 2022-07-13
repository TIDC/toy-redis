#pragma once

#include "base/marco.hpp"
#include "net/default_poller.hpp"
#include "net/io_service.hpp"

#include <string_view>

/// 服务端实现
class ToyRedisServer
{
    using IOServiceType = net::IOService<net::DefaultPoller>;
    /// 服务端配置信息
    struct ServerConfig
    {
        int32_t dbnum; // 最大数据库数量
        int32_t port;  // 服务端口
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
        auto befer_sleep = [&](auto &ios) {
            BeferSleep(ios);
        };
        io_service_.SetBeforeSleepCallback(befer_sleep);
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
    }

    void BeferSleep(IOServiceType &io_service)
    {
    }

private:
    ServerConfig config_;
    IOServiceType io_service_;
};