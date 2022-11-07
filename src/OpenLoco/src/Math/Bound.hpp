#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace OpenLoco::Math::Bound
{
    namespace Op
    {
        // Bounds an addition to within the capacity of type lhs preventing an overflow
        // Note: Does not work for 64bit or bigger fields
        // e.g. uint8_t a = 5; add(a, 254); a == 255; <--reaches upper limit
        struct Add
        {
            template<typename T, typename U>
            constexpr T operator()(T lhs, U rhs) const
            {
                static_assert(sizeof(T) < 8 && sizeof(U) < 8, "Bounded addition only possible with smaller than 64bit types");
                return static_cast<T>(std::min<int64_t>(static_cast<int64_t>(lhs) + rhs, std::numeric_limits<T>::max()));
            }
        };

        // Bounds a subtraction to within the capacity of type lhs preventing an underflow
        // Note: Does not work for 64bit or bigger fields
        // e.g. uint8_t a = 5; sub(a, 254); a == 0; <--reaches lower limit
        struct Sub
        {
            template<typename T, typename U>
            constexpr T operator()(T lhs, U rhs)
            {
                static_assert(sizeof(T) < 8 && sizeof(U) < 8, "Bounded subtraction only possible with smaller than 64bit types");
                return static_cast<T>(std::max<int64_t>(static_cast<int64_t>(lhs) - rhs, std::numeric_limits<T>::min()));
            }
        };
    }

    template<typename T, typename U>
    constexpr T add(T lhs, U rhs)
    {
        return Op::Add{}(lhs, rhs);
    }
    static_assert(add(static_cast<uint8_t>(5), 254) == 255);

    template<typename T, typename U>
    constexpr T sub(T lhs, U rhs)
    {
        return Op::Sub{}(lhs, rhs);
    }
    static_assert(sub(5u, 254u) == 0);
    static_assert(sub(5, 254) == -249);
    static_assert(sub(static_cast<int8_t>(5), 254) == -128);
}
