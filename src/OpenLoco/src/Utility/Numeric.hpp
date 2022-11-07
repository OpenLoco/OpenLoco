#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace OpenLoco::Utility
{
    int32_t bitScanForward(uint32_t source);

    int32_t bitScanReverse(uint32_t source);

    template<typename _UIntType>
    static constexpr _UIntType rol(_UIntType x, size_t shift)
    {
        static_assert(std::is_unsigned<_UIntType>::value, "result_type must be an unsigned integral type");
        using limits = typename std::numeric_limits<_UIntType>;
        return ((static_cast<_UIntType>(x) << shift) | (static_cast<_UIntType>(x) >> (limits::digits - shift)));
    }

    template<typename _UIntType>
    static constexpr _UIntType ror(_UIntType x, size_t shift)
    {
        static_assert(std::is_unsigned<_UIntType>::value, "result_type must be an unsigned integral type");
        using limits = std::numeric_limits<_UIntType>;
        return ((static_cast<_UIntType>(x) >> shift) | (static_cast<_UIntType>(x) << (limits::digits - shift)));
    }

    template<typename T>
    constexpr T setMask(T x, T mask, bool value)
    {
        return (value ? (x | mask) : (x & ~mask));
    }

    template<typename T>
    constexpr T setBit(T x, size_t index, bool value)
    {
        constexpr T mask = static_cast<T>(1 << index);
        return setMask(mask);
    }
}
