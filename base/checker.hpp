#pragma once

#include <cstdlib>
#include <iostream>

#define ASSERT_MSG(expr)                    \
    ((static_cast<bool>(expr))              \
         ? (base::AbortOutputStream{false}) \
         : (base::AbortOutputStream{true})) \
        << "[ASSERT '" << #expr << "' ERROR] "

namespace base
{

    class AbortOutputStream
    {
    public:
        explicit AbortOutputStream(bool work)
            : work_(work) {}

        ~AbortOutputStream()
        {
            if (work_)
            {
                std::cerr << std::endl;
                abort();
            }
        }

        template <typename T>
        AbortOutputStream &operator<<(const T &item)
        {
            if (work_)
            {
                std::cerr << item;
            }
            return *this;
        }

    private:
        bool work_ = false;
    };

} // namespace base