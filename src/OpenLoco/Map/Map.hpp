#pragma once

#include "../Math/Vector.hpp"
#include "../Types.hpp"

namespace OpenLoco::Map
{

    enum Inclusivity
    {
        Inclusive = 0,
        Exclusive,
    };

    using Bounds = std::pair<Inclusivity, int16_t>;

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

    using Pos2 = Math::Vector::TVector2<coord_t, 1>;
    using Pos3 = Math::Vector::TVector3<coord_t, 1>;
    using TilePos2 = Math::Vector::TVector2<coord_t, tile_size>;

    // Until interop is removed this is a requirement.
    static_assert(sizeof(Pos2) == 4);
    static_assert(sizeof(Pos3) == 6);
    static_assert(sizeof(TilePos2) == 4);

    constexpr bool isWithinLowerBound(const coord_t& coord, const Bounds& lowerBound)
    {
        return lowerBound.first ? coord > lowerBound.second : coord >= lowerBound.second;
    }

    constexpr bool isWithinUpperBound(const coord_t& coord, const Bounds& upperBound)
    {
        return upperBound.first ? coord < upperBound.second : coord <= upperBound.second;
    }

    constexpr bool validCoordWithinBounds(const coord_t& coord, const Bounds& lowerBound, const Bounds& upperBound)
    {
        return isWithinLowerBound(coord, lowerBound) && isWithinUpperBound(coord, upperBound);
    }

    constexpr bool validCoord(const coord_t& coord)
    {
        return validCoordWithinBounds(coord, { Inclusive, 0 }, { Exclusive, map_width });
    }

    constexpr bool validTileCoord(const coord_t& coord)
    {
        return validCoordWithinBounds(coord, { Inclusive, 0 }, { Exclusive, map_columns });
    }

    constexpr bool validCoordsWithinBounds(const Pos2& coords, const Bounds& lowerBound, const Bounds& upperBound)
    {
        return validCoordWithinBounds(coords.x, lowerBound, upperBound) && validCoordWithinBounds(coords.y, lowerBound, upperBound);
    }

    constexpr bool validCoords(const Pos2& coords)
    {
        return validCoord(coords.x) && validCoord(coords.y);
    }

    constexpr bool validCoords(const TilePos2& coords)
    {
        return validTileCoord(coords.x) && validTileCoord(coords.y);
    }
}
