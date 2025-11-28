#include "MapSelection.h"
#include "Input.h"
#include "Map/TileManager.h"
#include "Ui/ViewportInteraction.h"
#include <utility>

namespace OpenLoco::World
{
    static MapSelectionFlags _mapSelectionFlags = MapSelectionFlags::none; // 0x00F24484
    static coord_t _mapSelectionAX = 0;                                    // 0x00F24486
    static coord_t _mapSelectionBX = 0;                                    // 0x00F24488
    static coord_t _mapSelectionAY = 0;                                    // 0x00F2448A
    static coord_t _mapSelectionBY = 0;                                    // 0x00F2448C
    static MapSelectionType _mapSelectionType = MapSelectionType::corner0; // 0x00F2448E
    static ConstructionArrow _constructionArrow;                           // 0x00F24942 & 0x00F24948

    constexpr uint16_t kMapSelectedFreeFormTilesSize = 300;
    static sfl::static_vector<Pos2, kMapSelectedFreeFormTilesSize> _mapSelectedFreeFormTiles;

    // TODO: Return std::optional
    uint16_t setMapSelectionTiles(const Pos2& loc, const MapSelectionType selectionType, uint16_t toolSizeA)
    {
        uint16_t xPos = loc.x;
        uint16_t yPos = loc.y;
        uint8_t count = 0;

        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
        {
            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            count++;
        }

        if (_mapSelectionType != selectionType)
        {
            _mapSelectionType = selectionType;
            count++;
        }

        if (!toolSizeA)
        {
            toolSizeA = 1;
        }

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

        if (setQuadrant && _mapSelectionType != cursorQuadrant)
        {
            _mapSelectionType = cursorQuadrant;
            count++;
        }
        else if (!setQuadrant && _mapSelectionType != MapSelectionType::full)
        {
            _mapSelectionType = MapSelectionType::full;
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

    void resetMapSelectionFreeFormTiles()
    {
        _mapSelectedFreeFormTiles.clear();
    }

    void addMapSelectionFreeFormTile(const Pos2& pos)
    {
        _mapSelectedFreeFormTiles.push_back(pos);
    }

    std::span<const Pos2> getMapSelectionFreeFormTiles()
    {
        return _mapSelectedFreeFormTiles;
    }

    // 0x0046112C
    void mapInvalidateMapSelectionFreeFormTiles()
    {
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstruct))
        {
            return;
        }

        for (const auto& position : _mapSelectedFreeFormTiles)
        {
            TileManager::mapInvalidateTileFull(position);
        }
    }

    bool isWithinMapSelectionFreeFormTiles(const Pos2 pos)
    {
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstruct))
        {
            return false;
        }

        return std::ranges::find(_mapSelectedFreeFormTiles, pos) != _mapSelectedFreeFormTiles.end();
    }

    void setMapSelectionArea(const Pos2& locA, const Pos2& locB)
    {
        _mapSelectionAX = std::min(locA.x, locB.x);
        _mapSelectionAY = std::min(locA.y, locB.y);
        _mapSelectionBX = std::max(locA.x, locB.x);
        _mapSelectionBY = std::max(locA.y, locB.y);
    }

    std::pair<Pos2, Pos2> getMapSelectionArea()
    {
        return std::make_pair(Pos2{ _mapSelectionAX, _mapSelectionAY }, Pos2{ _mapSelectionBX, _mapSelectionBY });
    }

    void setMapSelectionCorner(const MapSelectionType corner)
    {
        _mapSelectionType = corner;
    }

    MapSelectionType getMapSelectionCorner()
    {
        return _mapSelectionType;
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

    const ConstructionArrow& getConstructionArrow()
    {
        return _constructionArrow;
    }

    void setConstructionArrow(const ConstructionArrow& arrow)
    {
        _constructionArrow = arrow;
    }
}
