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
    constexpr int16_t kMicroZStep = 16;       // e.g. SurfaceElement::water is a microZ
    constexpr int16_t kMicroToSmallZStep = 4; // e.g. for comparisons between water and baseZ
    constexpr int16_t kSmallZStep = 4;        // e.g. TileElement::baseZ is a smallZ
    using SmallZ = uint8_t;
    using MicroZ = uint8_t;

    constexpr coord_t tileFloor(coord_t coord)
    {
        return coord & (tile_size - 1);
    }

    using Pos2 = Math::Vector::TVector2<coord_t, 1>;
    using Pos3 = Math::Vector::TVector3<coord_t, 1>;
    using TilePos2 = Math::Vector::TVector2<coord_t, tile_size>;

    // Until interop is removed this is a requirement.
    static_assert(sizeof(Pos2) == 4);
    static_assert(sizeof(Pos3) == 6);
    static_assert(sizeof(TilePos2) == 4);

    constexpr bool validCoord(const coord_t coord)
    {
        return (coord >= 0) && (coord < map_width);
    }

    constexpr bool validTileCoord(const coord_t coord)
    {
        return (coord >= 0) && (coord < map_columns);
    }

    constexpr bool validCoords(const Pos2& coords)
    {
        return validCoord(coords.x) && validCoord(coords.y);
    }

    constexpr bool validCoords(const TilePos2& coords)
    {
        return validTileCoord(coords.x) && validTileCoord(coords.y);
    }

    // drawing coordinates validation differs from general valid coordinate validation
    constexpr bool drawableCoord(const coord_t coord)
    {
        return (coord >= (Map::tile_size - 1)) && (coord < (Map::map_width - Map::tile_size));
    }

    constexpr bool drawableCoords(const Pos2& coords)
    {
        return drawableCoord(coords.x) && drawableCoord(coords.y);
    }
}
