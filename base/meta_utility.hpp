#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace base
{

    template <uint64_t N, typename T>
    struct less_or_equal
    {
        constexpr static bool value = N <= std::numeric_limits<T>::max();
    };

    template <uint64_t N, typename T>
    constexpr bool less_or_equal_v = less_or_equal<N, T>::value;

    template <size_t N, typename U, typename... T>
    struct least_uint_helper : std::conditional<less_or_equal_v<N, U>, U, typename least_uint_helper<N, T...>::type>
    {
    };

    template <size_t N, typename U>
    struct least_uint_helper<N, U> : std::conditional<less_or_equal_v<N, U>, U, void>
    {
    };

    template <size_t Naturals>
    struct least_uint : least_uint_helper<Naturals, uint8_t, uint16_t, uint32_t, uint64_t>
    {
    };

    /// @brief 自动推导能够存放数值 Naturals 的最小无符号整型。
    /// 当 N 取值 [0, 255] 时，least_uint_t<N> 的实际类型为 uint8_t；
    /// 当 N 取值 [256, 65535] 时，least_uint_t<N> 的实际类型为 uint16_t；
    /// 更大的值同理
    template <size_t Naturals>
    using least_uint_t = typename least_uint<Naturals>::type;

} // namespace base
