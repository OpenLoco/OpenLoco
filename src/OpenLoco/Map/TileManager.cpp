#include "TileManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../ViewportManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::TileManager
{
    static loco_global<tile_element* [0x30004], 0x00E40134> _tiles;
    static loco_global<coord_t, 0x00F24486> _mapSelectionAX;
    static loco_global<coord_t, 0x00F24488> _mapSelectionBX;
    static loco_global<coord_t, 0x00F2448A> _mapSelectionAY;
    static loco_global<coord_t, 0x00F2448C> _mapSelectionBY;

    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<map_pos[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;

    // 0x00461179
    void initialise()
    {
        call(0x00461179);
    }

    tile get(map_pos pos)
    {
        return get(pos.x, pos.y);
    }

    tile get(coord_t x, coord_t y)
    {
        tile_coord_t tileX = x / 32;
        tile_coord_t tileY = y / 32;

        size_t index = (tileY << 9) | tileX;
        auto data = _tiles[index];
        if (data == (tile_element*)0xFFFFFFFF)
        {
            data = nullptr;
        }
        return tile(tileX, tileY, data);
    }

    TileHeight getHeight(const map_pos& pos)
    {
        return get(pos).getHeight();
    }

    // 0x004610F2
    void mapInvalidateSelectionRect()
    {
        if ((Input::getMapSelectionFlags() & MapSelectFlag::enable) != 0)
        {
            for (coord_t x = _mapSelectionAX; x <= _mapSelectionBX; x += 32)
            {
                for (coord_t y = _mapSelectionAY; y <= _mapSelectionBY; y += 32)
                {
                    mapInvalidateTileFull({ x, y });
                }
            }
        }
    }

    // 0x004CBE5F
    // regs.ax: pos.x
    // regs.cx: pos.y
    void mapInvalidateTileFull(Map::map_pos pos)
    {
        Ui::ViewportManager::invalidate(pos, 0, 1120, ZoomLevel::eighth);
    }

    // 0x0046112C
    void mapInvalidateMapSelectionTiles()
    {
        if ((Input::getMapSelectionFlags() & MapSelectFlag::enableConstruct) == 0)
            return;

        for (uint16_t index = 0; index < mapSelectedTilesSize; ++index)
        {
            auto& position = _mapSelectedTiles[index];
            if (position.x == -1)
                break;
            mapInvalidateTileFull(position);
        }
    }
}
