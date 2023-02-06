//
// Created by innoyiya on 2022/7/2.
//
#pragma once

#include "base/marco.hpp"
#include "base/ring_queue.hpp"
#include "net/constants.hpp"
#include "net/poller_types.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>

#include <ostream>
#include <sys/epoll.h>

namespace net
{
    class EpollPoller
    {

    public:
        DISABLE_COPY(EpollPoller)

        EpollPoller()
        {
            epfd_ = epoll_create(1024);
        }

        int32_t AddEvent(int32_t fd, int32_t events)
        {

            if (epfd_ == -1)
                return -1;

            int op = fdEvent[fd].events == Event::None ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
            epoll_event ee;
            ee.events = 0;
            // 合并之前的事件
            events |= fdEvent[fd].events;
            if (events & Event::Read)
                ee.events |= EPOLLIN;
            if (events & Event::Write)
                ee.events |= EPOLLOUT;
            std::cout << "add event fd is " << fd << " and events is " << ee.events << std::endl;
            ee.data.fd = fd;
            // 判断是否加入成功
            if (epoll_ctl(epfd_, op, fd, &ee) == -1)
                return -1;
            if (fd > max_fd_)
                max_fd_ = fd;
            fdEvent[fd].events = events;
            return 0;
        }

        int32_t DeleteEvent(int32_t fd, int32_t events)
        {
            if (epfd_ == -1)
                return -1;
            epoll_event ee;
            int mask = fdEvent[fd].events & (~events);

            ee.events = 0;

            if (mask & Event::Read)
                ee.events |= EPOLLIN;
            if (mask & Event::Write)
                ee.events |= EPOLLOUT;

            ee.data.fd = fd;
            if (mask != Event::None)
            {
                epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee);
            }
            else
            {
                epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ee);
            }
            fdEvent[fd].events = mask;
            return 0;
        }

        int32_t Poll(uint64_t timeout_ms)
        {
            int retval, numevents = 0;

            retval = epoll_wait(epfd_, eEvents, MAX_NUMBER_OF_FD, timeout_ms);
            if (retval > 0)
            {

                numevents = retval;
                for (int i = 0; i < numevents; i++)
                {
                    int mask = Event::None;
                    epoll_event *e = &eEvents[i];

                    if (e->events & EPOLLIN)
                        mask |= Event::Read;
                    if (e->events & EPOLLOUT)
                        mask |= Event::Write;
                    // if (fired_fds_.)
                    fired_fds_.EmplaceBack((int32_t)e->data.fd, mask);
                }
            }
            return numevents;
        }

        const struct FiredEvent & GetFdEventByFd(int32_t fd){
            // fixme: 入参校验
            return fdEvent[fd];
        }

        template <std::invocable<FiredEvent> Consumer>
        void ConsumeAll(Consumer fn)
        {
            while (!fired_fds_.Empty())
            {
                fn(fired_fds_.Front());
                fired_fds_.Pop();
            }
        }

        std::optional<FiredEvent> Consume()
        {
            if (fired_fds_.Empty())
            {
                return std::nullopt;
            }

            auto result = std::make_optional(fired_fds_.Front());
            fired_fds_.Pop();
            return result;
        }

    private:
        int epfd_{-1};
        int max_fd_{-1};
        struct FiredEvent fdEvent[MAX_NUMBER_OF_FD];
        struct epoll_event eEvents[MAX_NUMBER_OF_FD];
        // epoll_wait() 返回后触发了 io 事件的 fd 队列
        base::RingQueue<FiredEvent, MAX_NUMBER_OF_FD> fired_fds_;
    };
} // namespace net