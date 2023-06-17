#pragma once

#include "Traits.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace OpenLoco::Numerics
{
    int32_t bitScanForward(uint32_t source);

    int32_t bitScanReverse(uint32_t source);

    template<typename _UIntType>
    static constexpr _UIntType rol(_UIntType x, size_t shift)
    {
        // C++20 replace with std::rotl
        using T = typename Traits::UnderlyingType<_UIntType>::type;
        static_assert(std::is_unsigned_v<T>, "result_type must be an unsigned integral type");
        using limits = typename std::numeric_limits<T>;
        const auto amount = shift & (limits::digits - 1);
        const auto upperShift = limits::digits - amount;
        if (amount == 0)
        {
            return x;
        }
        return static_cast<_UIntType>(((static_cast<T>(x) << amount) | (static_cast<T>(x) >> upperShift)));
    }

    template<typename _UIntType>
    static constexpr _UIntType ror(_UIntType x, size_t shift)
    {
        // C++20 replace with std::rotr
        using T = typename Traits::UnderlyingType<_UIntType>::type;
        static_assert(std::is_unsigned_v<T>, "result_type must be an unsigned integral type");
        using limits = std::numeric_limits<T>;
        const auto amount = shift & (limits::digits - 1);
        const auto upperShift = limits::digits - amount;
        if (amount == 0)
        {
            return x;
        }
        return static_cast<_UIntType>(((static_cast<_UIntType>(x) >> amount) | (static_cast<_UIntType>(x) << upperShift)));
    }

    template<typename T>
    constexpr T setMask(T x, T mask, bool value)
    {
        return (value ? (x | mask) : (x & ~mask));
    }
}
