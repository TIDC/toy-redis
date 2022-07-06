#include "base/checker.hpp"

#include <array>
#include <cerrno>
#include <cstdint>
#include <string_view>

#include <unistd.h>

namespace ipc
{
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
                << ErrorMessage(errno);
        }

        ~SimplePipeline()
        {
            CloseWriteEnd();
            CloseReadEnd();
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

        /// 关闭写入端
        void CloseWriteEnd()
        {
            close(WriteEnd());
        }

        /// 关闭读取端
        void CloseReadEnd()
        {
            close(ReadEnd());
        }

    private:
        std::string_view ErrorMessage(int32_t error_code)
        {
            switch (error_code)
            {
            case EFAULT:
                return "EFAULT: pipefd is not valid.";
            case EINVAL:
                return "EINVAL: Invalid value in flags.";
            case EMFILE:
                return "EMFILE: The per-process limit on "
                       "the number of open file descriptors has been reached.";
            case ENFILE:
                return "ENFILE: The system-wide limit on "
                       "the total number of open files has been reached.";
            default:
                return "UNKNOW ERROR";
            }
        }

        std::array<int32_t, 2> pipefd_;
    };

    /// TODO 消息管道，传输固定类型的消息
    template <typename MessageType>
    class MessagePipeline;

} // namespace ipc