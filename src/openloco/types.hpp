#pragma once

#include <cstdint>

namespace openloco
{
    template<typename T>
    struct location
    {
        T x = 0;
        T y = 0;
        T z = 0;

        location() = default;
        location(T locX, T locY, T locZ)
            : x(locX)
            , y(locY)
            , z(locZ)
        {
        }
    };

    using loc16 = location<int16_t>;
}
