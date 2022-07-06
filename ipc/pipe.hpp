#include "base/checker.hpp"
#include "base/errno_to_string.hpp"

#include <array>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <string_view>

#include <unistd.h>

namespace ipc
{
    enum class PipePort
    {
        ReadEnd = 0,
        WriteEnd = 1
    };

    /// pipe() 简易封装，构造时创建管道，析构时自动关闭
    class SimplePipeline
    {
    public:
        SimplePipeline()
        {
            auto result = pipe(pipefd_.data());
            ASSERT_MSG(result == 0)
                << "pipe() 创建管道失败, "
                << "errno=" << errno << ", "
                << base::ErrnoToString(errno);
        }

        ~SimplePipeline()
        {
            CloseAll();
        }

        /// 获取写入端的 fd
        int32_t WriteEnd() const
        {
            return pipefd_[1];
        }

        /// 获取读取端的 fd
        int32_t ReadEnd() const
        {
            return pipefd_[0];
        }

        /// 获取指定端口的 fd
        int32_t Get(PipePort port)
        {
            ASSERT_MSG(port == PipePort::ReadEnd ||
                       port == PipePort::WriteEnd)
                << "无效参数，port=" << static_cast<size_t>(port);

            if (port == PipePort::ReadEnd)
            {
                return pipefd_[0];
            }
            if (port == PipePort::WriteEnd)
            {
                return pipefd_[1];
            }
            return -1;
        }

        /// 设置指定端口是否阻塞
        void SetNotWait(PipePort port, bool is_block)
        {
            auto fd = Get(port);
            auto flags = fcntl(fd, F_GETFL);
            if (is_block)
            {
                flags |= O_NONBLOCK;
            }
            else
            {
                flags &= ~O_NONBLOCK;
            }
            auto result = fcntl(fd, F_SETFL, flags);
            ASSERT_MSG(result != -1)
                << "fcntl() 设置是否阻塞出错，errno=" << errno << "，"
                << base::ErrnoToString(errno);
        }

        /// 关闭指定端口
        void Close(PipePort port)
        {
            if (port == PipePort::ReadEnd)
            {
                close(ReadEnd());
            }
            if (port == PipePort::WriteEnd)
            {
                close(WriteEnd());
            }
        }

        /// 完全关闭管道
        void CloseAll()
        {
            Close(PipePort::ReadEnd);
            Close(PipePort::WriteEnd);
        }

    private:
        std::array<int32_t, 2> pipefd_;
    };

    /// TODO 消息管道，传输固定类型的消息
    template <typename MessageType>
    class MessagePipeline;

} // namespace ipc