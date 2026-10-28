#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace OpenLoco
{
    // To be replaced with std::to_underlying in c++23
    template<typename TEnum>
    constexpr auto enumValue(TEnum enumerator) noexcept
    {
        return static_cast<std::underlying_type_t<TEnum>>(enumerator);
    }

    // World space coordinates. These are 32 bit so that maps larger than 1024 tiles
    // (i.e. wider than 32768 world units) can be represented.
    using coord_t = int32_t;
    // Tile space coordinates. A tile is 32 world units, so 16 bit is enough for any map size.
    using tile_coord_t = int16_t;
}
