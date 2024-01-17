#pragma once

#include "TileElement.h"
#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>

namespace OpenLoco::Ui
{
    struct viewport_pos;
}

namespace OpenLoco::World
{
    struct TileHeight
    {
        coord_t landHeight;
        coord_t waterHeight;

        explicit operator coord_t() const
        {
            return waterHeight == 0 ? landHeight : waterHeight;
        }
    };

    // 0x004F9296, 0x004F9298
    constexpr std::array<Pos2, 4> kOffsets = {
        Pos2{ 0, 0 },
        Pos2{ 0, 32 },
        Pos2{ 32, 32 },
        Pos2{ 32, 0 },
    };

    // 0x00503C6C, 0x00503C6E
    constexpr std::array<Pos2, 16> kRotationOffset = {
        Pos2{ -32, 0 },
        Pos2{ 0, 32 },
        Pos2{ 32, 0 },
        Pos2{ 0, -32 },
        Pos2{ -32, 0 },
        Pos2{ 0, 32 },
        Pos2{ 32, 0 },
        Pos2{ 0, -32 },
        Pos2{ -32, 0 },
        Pos2{ 0, 32 },
        Pos2{ 32, 0 },
        Pos2{ 0, -32 },
        Pos2{ -32, 32 },
        Pos2{ 32, 32 },
        Pos2{ 32, -32 },
        Pos2{ -32, -32 },
    };

    // 0x00503CAC
    constexpr std::array<uint8_t, 16> kReverseRotation = {
        2,
        3,
        0,
        1,
        10,
        11,
        8,
        9,
        6,
        7,
        4,
        5,
        14,
        15,
        12,
        13,
    };

    struct LessThanPos3
    {
        bool operator()(World::Pos3 const& lhs, World::Pos3 const& rhs) const
        {
            return std::tie(lhs.x, lhs.y, lhs.z) < std::tie(rhs.x, rhs.y, rhs.z);
        }
    };

    Ui::viewport_pos gameToScreen(const Pos3& loc, int rotation);

    struct SurfaceElement;
    struct StationElement;

    struct Tile
    {
    private:
        TileElement* const _data;

    public:
        static constexpr size_t npos = std::numeric_limits<size_t>().max();

        const TilePos2 pos;

        Tile(const TilePos2& tPos, TileElement* data);
        bool isNull() const;
        TileElement* begin();
        TileElement* begin() const;
        TileElement* end();
        TileElement* end() const;
        size_t size();
        TileElement* operator[](size_t i);

        size_t indexOf(const TileElementBase* element) const;
        SurfaceElement* surface() const;
        StationElement* trainStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const;
        StationElement* roadStation(uint8_t roadId, uint8_t direction, uint8_t baseZ) const;
    };
}
