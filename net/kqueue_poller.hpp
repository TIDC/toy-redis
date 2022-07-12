#include "base/marco.hpp"
#include "base/ring_queue.hpp"
#include "net/constants.hpp"
#include "net/poller_types.hpp"

#include <optional>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

namespace net
{
    /// select 封装
    class KQueuePoller
    {
    public:
        DISABLE_COPY(KQueuePoller);

        KQueuePoller()
        {
            kqfd_ = kqueue();
            events_ = std::make_unique<struct kevent[]>(1024 * 10);
            if (kqfd_ == -1)
                return;
        }

        /// 新增事件监听
        int32_t AddEvent(int32_t fd, int32_t events)
        {
            struct kevent ke;
            if (events & Event::Read || events & Event::Write)
            {
                if (events & Event::Read)
                    EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                if (events & Event::Write)
                    EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);

                if (kevent(kqfd_, &ke, 1, nullptr, 0, nullptr) == -1)
                    return -1;
            }
            return 0;
        }

        /// 移除事件监听
        void DeleteEvent(int32_t fd, int32_t events)
        {
            struct kevent ke;
            if (events & Event::Read || events & Event::Write)
            {
                if (events & Event::Read)
                    EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                if (events & Event::Write)
                    EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);

                kevent(kqfd_, &ke, 1, nullptr, 0, nullptr);
            }
        }

        int32_t Poll(uint64_t timeout_ms)
        {
            assert(timeout_ms >= 0);
            // 传递给 select() 用的 fd_set 是一次性的，想要安全重用就得复制一份给 select()
            struct timespec timeout
            {
                .tv_sec = static_cast<int64_t>(timeout_ms / 1000),
                .tv_nsec = static_cast<__darwin_suseconds_t>(static_cast<int64_t>(timeout_ms % 1000) * 1000)
            };

            int32_t result = kevent(kqfd_, nullptr, 0, events_.get(), FD_SETSIZE, &timeout);
            int32_t numevents = 0;

            if (result > 0)
            {
                numevents = result;

                for (int32_t i = 0; i < numevents; i++)
                {
                    int mask = 0;
                    auto event = events_[i];
                    if (event.filter == EVFILT_READ)
                        mask |= Event::Read;
                    if (event.filter == EVFILT_WRITE)
                        mask |= Event::Write;
                    if (mask != 0)
                        fired_fds_.EmplaceBack((int32_t)event.ident, mask);
                }
            }
            return numevents;
        }

        /// 传入一个可调用实例，处理全部触发了读写事件的 fd
        /// 参数 fn 的类型是 void(FiredEvent)
        template <std::invocable<FiredEvent> Consumer>
        void ConsumeAll(Consumer fn)
        {
            while (!fired_fds_.Empty())
            {
                fn(fired_fds_.Front());
                fired_fds_.Pop();
            }
        }

        /// 获取一个触发了读写事件的 fd
        std::optional<FiredEvent> Consume()
        {
            if (fired_fds_.Empty())
                return std::nullopt;

            auto result = std::make_optional(fired_fds_.Front());
            fired_fds_.Pop();
            return result;
        }

    private:
        int kqfd_;
        std::unique_ptr<struct kevent[]> events_ = nullptr;
        base::RingQueue<FiredEvent, MAX_NUMBER_OF_FD> fired_fds_;
    };
} // namespace net