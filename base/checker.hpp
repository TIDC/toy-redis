#pragma once

#include <cstdlib>
#include <initializer_list>
#include <iostream>

#define ASSERT_MSG(expr)                                       \
    ((static_cast<bool>(expr))                                 \
         ? (base::AbortOutputStream{false})                    \
         : (base::AbortOutputStream{true}))                    \
        .Print("[!!!!!! ASSERT '", #expr, "' ERROR !!!!!!]\n") \
        .Print("location: ", __FILE__, ":", __LINE__, " ", __func__, "\n")

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
                output_count_++;
                if (output_count_ == 1)
                {
                    Print("what: ");
                }

                Print(item);
            }
            return *this;
        }

        template <typename T, typename... Ts>
        AbortOutputStream &Print(const T &item, const Ts... items)
        {
            if (work_)
            {
                std::cerr << item;
                if (sizeof...(items) > 0)
                {
                    Print(items...);
                }
            }
            return *this;
        }

        template <typename T>
        AbortOutputStream &Print(const T &item)
        {
            if (work_)
            {
                std::cerr << item;
            }
            return *this;
        }

    private:
        bool work_ = false;
        size_t output_count_ = 0;
    };

} // namespace base