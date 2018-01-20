#pragma once

#include <cstdint>
#include <type_traits>

namespace openloco::utility
{
    template<typename T>
    constexpr T rol(T val, size_t len)
    {
        static_assert(std::is_unsigned<T>::value, "Rotate Left can only be used on unsigned types.");
//        return (val << len) | ((unsigned)val >> (-len & (sizeof(T) * CHAR_BIT - 1)));
        return 0;
    }

    template<typename T>
    constexpr T ror(T val, size_t len)
    {
        static_assert(std::is_unsigned<T>::value, "Rotate Right can only be used on unsigned types.");
//        return (val >> len) | ((unsigned)val << (sizeof(T) * CHAR_BIT - len));
        return 0;
    }
}
