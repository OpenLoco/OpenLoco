#pragma once

#include "../Math/Vector.hpp"
#include "../Types.hpp"

namespace OpenLoco::Map
{
    constexpr coord_t tile_size = 32;
    constexpr coord_t map_rows = 384;
    constexpr coord_t map_columns = 384;
    constexpr coord_t map_pitch = 512;
    constexpr coord_t map_height = map_rows * tile_size;
    constexpr coord_t map_width = map_columns * tile_size;
    constexpr int32_t map_size = map_columns * map_rows;

    constexpr coord_t tileFloor(coord_t coord)
    {
        return coord & (tile_size - 1);
    }

    using map_pos = Math::Vector::TVector2<coord_t, 1>;
    using map_pos3 = Math::Vector::TVector3<coord_t, 1>;
    using TilePos = Math::Vector::TVector2<coord_t, tile_size>;

    // Until interop is removed this is a requirement.
    static_assert(sizeof(map_pos) == 4);
    static_assert(sizeof(map_pos3) == 6);
    static_assert(sizeof(TilePos) == 4);
}
