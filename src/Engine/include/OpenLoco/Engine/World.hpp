#pragma once

#include "Types.hpp"
#include <OpenLoco/Math/Vector.hpp>
#include <algorithm>

namespace OpenLoco::World
{
    constexpr coord_t kTileSize = 32;

    constexpr coord_t kMapPitch = 512;
    constexpr int16_t kMicroZStep = 16;       // e.g. SurfaceElement::water is a microZ
    constexpr int16_t kMicroToSmallZStep = 4; // e.g. for comparisons between water and baseZ
    constexpr int16_t kSmallZStep = 4;        // e.g. TileElement::baseZ is a smallZ
    using SmallZ = uint8_t;
    using MicroZ = uint8_t;

    constexpr coord_t tileFloor(coord_t coord)
    {
        return coord & (kTileSize - 1);
    }

    constexpr int16_t heightFloor(int16_t height)
    {
        return height & ~(kSmallZStep - 1);
    }
    static_assert(heightFloor(0x62) == 0x60);

    struct WorldSpaceTag
    {
    };
    using Pos2 = Math::Vector::TVector2<coord_t, WorldSpaceTag>;
    static_assert(std::is_trivially_copyable_v<Pos2>, "Pos2 must be POD");

    using Pos3 = Math::Vector::TVector3<coord_t, WorldSpaceTag>;
    static_assert(std::is_trivially_copyable_v<Pos3>, "Pos2 must be POD");

    struct TileSpaceTag
    {
    };
    using TilePos2 = Math::Vector::TVector2<tile_coord_t, TileSpaceTag>;
    static_assert(std::is_trivially_copyable_v<TilePos2>, "Pos2 must be POD");

    // Until interop is removed this is a requirement.
    static_assert(sizeof(Pos2) == 4);
    static_assert(sizeof(Pos3) == 6);
    static_assert(sizeof(TilePos2) == 4);

    constexpr TilePos2 toTileSpace(const Pos2& coords)
    {
        return TilePos2{ static_cast<tile_coord_t>(coords.x / kTileSize), static_cast<tile_coord_t>(coords.y / kTileSize) };
    }

    constexpr TilePos2 toTileSpace(const Pos3& coords)
    {
        return TilePos2{ static_cast<tile_coord_t>(coords.x / kTileSize), static_cast<tile_coord_t>(coords.y / kTileSize) };
    }

    constexpr Pos2 toWorldSpace(const TilePos2& coords)
    {
        return Pos2{ static_cast<coord_t>(coords.x * kTileSize), static_cast<coord_t>(coords.y * kTileSize) };
    }
}
