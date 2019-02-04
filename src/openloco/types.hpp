#pragma once

#include <cstdint>

namespace openloco
{
    template<typename T>
    struct location2
    {
        T x = 0;
        T y = 0;

        location2() = default;
        location2(T locX, T locY)
            : x(locX)
            , y(locY)
        {
        }
    };

    template<typename T>
    struct location3
    {
        T x = 0;
        T y = 0;
        T z = 0;

        location3() = default;
        location3(T locX, T locY, T locZ)
            : x(locX)
            , y(locY)
            , z(locZ)
        {
        }
    };

    using xy32 = location2<int32_t>;
    using loc16 = location3<int16_t>;

    constexpr int32_t speed16_to_32(int32_t speed_mph) { return speed_mph << 16; }
    namespace literals
    {
        constexpr uint32_t operator"" _mph32(unsigned long long speed_mph)
        {
            return speed_mph << 16;
        }
    }
}
