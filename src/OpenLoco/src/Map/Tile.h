#pragma once

#include "Types.hpp"
#include "Map.hpp"
#include "TileElement.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace OpenLoco::Ui
{
    struct viewport_pos;
}

namespace OpenLoco::Map
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

    // 0x004F9296, 0x4F9298
    constexpr Pos2 offsets[4] = { { 0, 0 }, { 0, 32 }, { 32, 32 }, { 32, 0 } };

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
        StationElement* trackStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const;
        StationElement* roadStation(uint8_t roadId, uint8_t direction, uint8_t baseZ) const;
    };
}
