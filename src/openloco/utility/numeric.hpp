#pragma once

#include <cstddef>
#include <cstdint>
#include <limits.h>
#include <type_traits>

namespace openloco::utility
{
    int32_t bitscanforward(uint32_t source);

    template<typename T>
    constexpr T rol(T val, size_t len)
    {
        static_assert(std::is_unsigned<T>::value, "Rotate Left can only be used on unsigned types.");
        return (val << len) | ((unsigned)val >> (-len & (sizeof(T) * CHAR_BIT - 1)));
    }

    template<typename T>
    constexpr T ror(T val, size_t len)
    {
        static_assert(std::is_unsigned<T>::value, "Rotate Right can only be used on unsigned types.");
        return (val >> len) | ((unsigned)val << (sizeof(T) * CHAR_BIT - len));
    }

    template<typename T>
    constexpr T set_mask(T x, T mask, bool value)
    {
        return (value ? (x | mask) : (x & ~mask));
    }

    template<typename T>
    constexpr T set_bit(T x, size_t index, bool value)
    {
        constexpr T mask = static_cast<T>(1 << index);
        return set_mask(mask);
    }
}
