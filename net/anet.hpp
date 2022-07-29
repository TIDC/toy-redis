//
// Created by innoyiya on 2022/7/28.
//
#pragma once
#include "base/marco.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

namespace net {
    class RedisNet
    {
    public:
        DISABLE_COPY(RedisNet)

        // 创建tcp服务
        int anetTcpServer(char *err, int port, char *bindaddr)
        {
            int s;
            struct sockaddr_in sa;

            if ((s = anetCreateSocket(err, AF_INET)) == ANET_ERR) {
                return ANET_ERR;
            }

            memset(&sa,0,sizeof(sa));
            sa.sin_family = AF_INET;
            sa.sin_port = htons(port);
            sa.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bindaddr && inet_aton(bindaddr, &sa.sin_addr) == 0) {
                anetSetError(err, "invalid bind address");
                shutdown(s, 2);
                return ANET_ERR;
            }
            if (anetListen(err,s,(struct sockaddr*)&sa,sizeof(sa)) == ANET_ERR)
                return ANET_ERR;
            return s;
        }

        // 接受建立tcp链接
        int anetTcpAccept(char *err, int s, char *ip, int *port) {
            int fd;
            struct sockaddr_in sa;
            socklen_t salen = sizeof(sa);
            if ((fd = anetGenericAccept(err,s,(struct sockaddr*)&sa,&salen)) == ANET_ERR)
                return ANET_ERR;

            if (ip) strcpy(ip,inet_ntoa(sa.sin_addr));
            if (port) *port = ntohs(sa.sin_port);
            return fd;
        }

    private:
        int anetCreateSocket(char *err, int domain) {
            int s, on = 1;
            if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
                anetSetError(err, "creating socket: %s", strerror(errno));
                return ANET_ERR;
            }
            // 避免重启tcp服务的时候出现Address in use 的错误
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
                anetSetError(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
            }
        }

        void anetSetError(char *err, const char *fmt, ...) {
            va_list ap;

            if (!err) return;
            va_start(ap, fmt);
            vsnprintf(err, ANET_ERR_LEN, fmt, ap);
            va_end(ap);
        }

        int anetGenericAccept(char *err, int s, struct sockaddr *sa, socklen_t *len) {
            int fd;
            while(1) {
                fd = accept(s,sa,len);
                if (fd == -1) {
                    if (errno == EINTR)
                        continue;
                    else {
                        anetSetError(err, "accept: %s", strerror(errno));
                        return ANET_ERR;
                    }
                }
                break;
            }
            return fd;
        }

        int anetListen(char *err, int s, struct sockaddr *sa, socklen_t len) {
            if (bind(s,sa,len) == -1) {
                anetSetError(err, "bind: %s", strerror(errno));
                shutdown(s, 2);
                return ANET_ERR;
            }
            if (listen(s, 511) == -1) { /* the magic 511 constant is from nginx */
                anetSetError(err, "listen: %s", strerror(errno));
                shutdown(s, 2);
                return ANET_ERR;
            }
            return ANET_OK;
        }

    private:
        int ANET_ERR_LEN = 256;
        int ANET_ERR = -1;
        int ANET_OK = 0;
    };
}