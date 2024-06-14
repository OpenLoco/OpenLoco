#include "MapSelection.h"
#include "Input.h"
#include "Map/TileManager.h"
#include "Ui/ViewportInteraction.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <utility>

using namespace OpenLoco::Interop;

namespace OpenLoco::World
{
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;

    static loco_global<MapSelectionFlags, 0x00F24484> _mapSelectionFlags;
    static loco_global<coord_t, 0x00F24486> _mapSelectionAX;
    static loco_global<coord_t, 0x00F24488> _mapSelectionBX;
    static loco_global<coord_t, 0x00F2448A> _mapSelectionAY;
    static loco_global<coord_t, 0x00F2448C> _mapSelectionBY;
    static loco_global<MapSelectionType, 0x00F2448E> _word_F2448E;

    constexpr uint16_t kMapSelectedTilesSize = 300;
    static loco_global<Pos2[kMapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;

    // TODO: Return std::optional
    uint16_t setMapSelectionTiles(const Pos2& loc, const MapSelectionType selectionType)
    {
        uint16_t xPos = loc.x;
        uint16_t yPos = loc.y;
        uint8_t count = 0;

        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
        {
            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            count++;
        }

        if (_word_F2448E != selectionType)
        {
            _word_F2448E = selectionType;
            count++;
        }

        uint16_t toolSizeA = _adjustToolSize;

        if (!toolSizeA)
            toolSizeA = 1;

        toolSizeA = toolSizeA << 5;
        uint16_t toolSizeB = toolSizeA;
        toolSizeB -= 32;
        toolSizeA = toolSizeA >> 1;
        toolSizeA -= 16;
        xPos -= toolSizeA;
        yPos -= toolSizeA;
        xPos &= 0xFFE0;
        yPos &= 0xFFE0;

        if (xPos != _mapSelectionAX)
        {
            _mapSelectionAX = xPos;
            count++;
        }

        if (yPos != _mapSelectionAY)
        {
            _mapSelectionAY = yPos;
            count++;
        }

        xPos += toolSizeB;
        yPos += toolSizeB;

        if (xPos != _mapSelectionBX)
        {
            _mapSelectionBX = xPos;
            count++;
        }

        if (yPos != _mapSelectionBY)
        {
            _mapSelectionBY = yPos;
            count++;
        }

        mapInvalidateSelectionRect();

        return count;
    }

    uint16_t setMapSelectionSingleTile(const Pos2& loc, bool setQuadrant)
    {
        uint16_t xPos = loc.x & 0xFFE0;
        uint16_t yPos = loc.y & 0xFFE0;
        MapSelectionType cursorQuadrant = getQuadrantOrCentreFromPos(loc);

        auto count = 0;
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
        {
            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            count++;
        }

        if (setQuadrant && _word_F2448E != cursorQuadrant)
        {
            _word_F2448E = cursorQuadrant;
            count++;
        }
        else if (!setQuadrant && _word_F2448E != MapSelectionType::full)
        {
            _word_F2448E = MapSelectionType::full;
            count++;
        }

        if (xPos != _mapSelectionAX)
        {
            _mapSelectionAX = xPos;
            count++;
        }

        if (yPos != _mapSelectionAY)
        {
            _mapSelectionAY = yPos;
            count++;
        }

        if (xPos != _mapSelectionBX)
        {
            _mapSelectionBX = xPos;
            count++;
        }

        if (yPos != _mapSelectionBY)
        {
            _mapSelectionBY = yPos;
            count++;
        }

        mapInvalidateSelectionRect();

        return count;
    }

    // 0x004610F2
    void mapInvalidateSelectionRect()
    {
        if (World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
        {
            for (coord_t x = _mapSelectionAX; x <= _mapSelectionBX; x += 32)
            {
                for (coord_t y = _mapSelectionAY; y <= _mapSelectionBY; y += 32)
                {
                    TileManager::mapInvalidateTileFull({ x, y });
                }
            }
        }
    }

    // 0x0046112C
    void mapInvalidateMapSelectionTiles()
    {
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstruct))
            return;

        for (uint16_t index = 0; index < kMapSelectedTilesSize; ++index)
        {
            auto& position = _mapSelectedTiles[index];
            if (position.x == -1)
                break;
            TileManager::mapInvalidateTileFull(position);
        }
    }

    void setMapSelectionArea(const Pos2& locA, const Pos2& locB)
    {
        _mapSelectionAX = locA.x;
        _mapSelectionAY = locA.y;
        _mapSelectionBX = locB.x;
        _mapSelectionBY = locB.y;
    }

    std::pair<Pos2, Pos2> getMapSelectionArea()
    {
        return std::make_pair(Pos2{ _mapSelectionAX, _mapSelectionAY }, Pos2{ _mapSelectionBX, _mapSelectionBY });
    }

    void setMapSelectionCorner(const MapSelectionType corner)
    {
        _word_F2448E = corner;
    }

    MapSelectionType getMapSelectionCorner()
    {
        return _word_F2448E;
    }

    // 0x0045FD8E
    // NOTE: Original call getSurfaceLocFromUi within this function
    // instead OpenLoco has split it in two. Also note that result of original
    // was a Pos2 start i.e. (& 0xFFE0) both components
    MapSelectionType getQuadrantOrCentreFromPos(const Pos2& loc)
    {
        // Determine to which quadrants the cursor is closest 4 == all quadrants
        const auto xNibbleCentre = std::abs((loc.x & 0xFFE0) + 16 - loc.x);
        const auto yNibbleCentre = std::abs((loc.y & 0xFFE0) + 16 - loc.y);
        if (std::max(xNibbleCentre, yNibbleCentre) <= 7)
        {
            // Is centre so all quadrants
            return MapSelectionType::full;
        }

        return getQuadrantFromPos(loc);
    }

    // 0x0045FE05
    // NOTE: Original call getSurfaceLocFromUi within this function
    // instead OpenLoco has split it in two. Also note that result of original
    // was a Pos2 start i.e. (& 0xFFE0) both components
    MapSelectionType getQuadrantFromPos(const Pos2& loc)
    {
        const auto xNibble = loc.x & 0x1F;
        const auto yNibble = loc.y & 0x1F;
        if (xNibble > 16)
        {
            return (yNibble >= 16) ? MapSelectionType::corner0 : MapSelectionType::corner1;
        }
        else
        {
            return (yNibble >= 16) ? MapSelectionType::corner3 : MapSelectionType::corner2;
        }
    }

    // 0x0045FE4C
    // NOTE: Original call getSurfaceLocFromUi within this function
    // instead OpenLoco has split it in two. Also note that result of original
    // was a Pos2 start i.e. (& 0xFFE0) both components
    MapSelectionType getSideFromPos(const Pos2& loc)
    {
        const auto xNibble = loc.x & 0x1F;
        const auto yNibble = loc.y & 0x1F;
        if (xNibble < yNibble)
        {
            return (xNibble + yNibble < 32) ? MapSelectionType::corner0 : MapSelectionType::corner1;
        }
        else
        {
            return (xNibble + yNibble < 32) ? MapSelectionType::corner3 : MapSelectionType::corner2;
        }
    }

    MapSelectionFlags getMapSelectionFlags()
    {
        return _mapSelectionFlags;
    }

    bool hasMapSelectionFlag(MapSelectionFlags flags)
    {
        return (_mapSelectionFlags & flags) != MapSelectionFlags::none;
    }

    void setMapSelectionFlags(MapSelectionFlags flags)
    {
        _mapSelectionFlags = _mapSelectionFlags | flags;
    }

    void resetMapSelectionFlag(MapSelectionFlags flags)
    {
        _mapSelectionFlags = _mapSelectionFlags & ~flags;
    }

    void resetMapSelectionFlags()
    {
        _mapSelectionFlags = MapSelectionFlags::none;
    }
}
