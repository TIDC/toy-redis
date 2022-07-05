//
// Created by innoyiya on 2022/7/2.
//
#pragma once

#include "base/marco.hpp"
#include "constants.hpp"
#include "poller_types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sys/epoll.h>
namespace net
{
    class EpollPoller
    {
        struct FdEvent
        {
            int32_t mask_ = Event::None;
        };
    public:
        DISABLE_COPY(EpollPoller)

        EpollPoller()
        {
            epfd_ = epoll_create(1024);
        }

        int32_t AddEvent(int32_t fd, int32_t events)
        {

            if (epfd_ == -1) return -1;

            int op = fdEvent[fd].mask_ == Event::None ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
            epoll_event ee;
            ee.events = 0;
            // 合并之前的事件
            events |= fdEvent[fd].mask_;
            if (events & Event::Read) ee.events |= EPOLLIN;
            if (events & Event::Write) ee.events |= EPOLLOUT;
            ee.data.fd = fd;
            // 判断是否加入成功
            if (epoll_ctl(epfd_, op, fd, &ee) == -1) return -1;
            if (fd > max_fd_) max_fd_ = fd;
            fdEvent[fd].mask_ |= events;
            return 0;
        }

        void DeleteEvent(int32_t fd, int32_t events)
        {
            epoll_event ee;
            int mask = fdEvent[fd].mask_ & (~events);

            ee.events = 0;

            if (mask & Event::Read) ee.events |= EPOLLIN;
            if (mask & Event::Write) ee.events |= EPOLLOUT;

            ee.data.fd = fd;
            if (mask != Event::None) {
                epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee);
            }else {
                epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ee);
            }
        }


    private:
        int epfd_{-1};
        int max_fd_{-1};
        struct FdEvent fdEvent[MAX_NUMBER_OF_FD];
        struct epoll_event eEvents[MAX_NUMBER_OF_FD];
    };
}