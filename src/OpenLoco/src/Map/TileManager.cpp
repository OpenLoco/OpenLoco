#include "TileManager.h"
#include "Audio/Audio.h"
#include "BuildingElement.h"
#include "CompanyManager.h"
#include "Entities/Misc.h"
#include "Game.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "IndustryElement.h"
#include "IndustryManager.h"
#include "Input.h"
#include "Objects/BuildingObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "QuarterTile.h"
#include "RoadElement.h"
#include "SurfaceElement.h"
#include "TownManager.h"
#include "TreeElement.h"
#include "Ui.h"
#include "ViewportManager.h"
#include "WallElement.h"
#include <OpenLoco/Engine/Map.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::TileManager
{
    static loco_global<TileElement*, 0x005230C8> _elements;
    static loco_global<TileElement* [0x30004], 0x00E40134> _tiles;
    static loco_global<TileElement*, 0x00F00134> _elementsEnd;
    static loco_global<TileElement*, 0x00F00158> _F00158;
    static loco_global<uint32_t, 0x00F00168> _F00168;
    static loco_global<coord_t, 0x00F24486> _mapSelectionAX;
    static loco_global<coord_t, 0x00F24488> _mapSelectionBX;
    static loco_global<coord_t, 0x00F2448A> _mapSelectionAY;
    static loco_global<coord_t, 0x00F2448C> _mapSelectionBY;
    static loco_global<uint16_t, 0x00F2448E> _word_F2448E;
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;
    static loco_global<Map::Pos2, 0x00525F6E> _startUpdateLocation;

    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<Pos2[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;

    static TileElement* InvalidTile = reinterpret_cast<TileElement*>(static_cast<intptr_t>(-1));

    // 0x0046902E
    void removeSurfaceIndustry(const Pos2& pos)
    {
        auto tile = get(pos);
        auto* surface = tile.surface();
        if (surface != nullptr)
        {
            surface->removeIndustry(pos);
        }
    }

    // 0x00461179
    void initialise()
    {
        _F00168 = 0;
        _startUpdateLocation = Map::Pos2(0, 0);
        const auto landType = getGameState().lastLandOption == 0xFF ? 0 : getGameState().lastLandOption;

        SurfaceElement defaultElement{};
        defaultElement.setTerrain(landType);
        defaultElement.setBaseZ(4);
        defaultElement.setLastFlag(true);

        auto* element = *_elements;
        for (auto i = 0; i < kMapSize; ++i, ++element)
        {
            *element = *reinterpret_cast<TileElement*>(&defaultElement);
        }
        updateTilePointers();
        getGameState().flags |= GameStateFlags::tileManagerLoaded;
    }

    stdx::span<TileElement> getElements()
    {
        return stdx::span<TileElement>(static_cast<TileElement*>(_elements), getElementsEnd());
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

    void setMapSelectionCorner(const uint8_t corner)
    {
        _word_F2448E = corner;
    }

    uint8_t getMapSelectionCorner()
    {
        return _word_F2448E;
    }

    TileElement* getElementsEnd()
    {
        return _elementsEnd;
    }

    uint32_t numFreeElements()
    {
        return maxElements - (_elementsEnd - _elements);
    }

    void setElements(stdx::span<TileElement> elements)
    {
        TileElement* dst = _elements;
        std::memset(dst, 0, maxElements * sizeof(TileElement));
        std::memcpy(dst, elements.data(), elements.size_bytes());
        TileManager::updateTilePointers();
    }

    // Note: Must be past the last tile flag
    static void markElementAsFree(TileElement& element)
    {
        element.setBaseZ(255);
        if (element.next() == *_elementsEnd)
        {
            _elementsEnd--;
        }
    }

    // 0x00461760
    void removeElement(TileElement& element)
    {
        // This is used to indicate if the caller can still use this pointer
        if (&element == *_F00158)
        {
            if (element.isLast())
            {
                *_F00158 = reinterpret_cast<TileElement*>(-1);
            }
        }

        if (element.isLast())
        {
            auto* prev = element.prev();
            prev->setLastFlag(true);
            markElementAsFree(element);
        }
        else
        {
            // Move all of the elments up one until last for the tile
            auto* next = element.next();
            auto* cur = &element;
            do
            {
                *cur++ = *next;
            } while (!next++->isLast());

            markElementAsFree(*cur);
        }
    }

    // 0x004616D6
    TileElement* insertElement(ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.bl = baseZ;
        regs.bh = occupiedQuads;
        call(0x004616D6, regs);
        TileElement* el = X86Pointer<TileElement>(regs.esi);
        el->setType(type);
        return el;
    }

    TileElement** getElementIndex()
    {
        return _tiles.get();
    }

    Tile get(TilePos2 pos)
    {
        size_t index = (pos.y << 9) | pos.x;
        auto data = _tiles[index];
        if (data == InvalidTile)
        {
            data = nullptr;
        }
        return Tile(pos, data);
    }

    Tile get(Pos2 pos)
    {
        return get(pos.x, pos.y);
    }

    Tile get(coord_t x, coord_t y)
    {
        return get(TilePos2(x / Map::kTileSize, y / Map::kTileSize));
    }

    constexpr uint8_t kTileSize = 31;

    static int16_t getOneCornerUpLandHeight(int8_t xl, int8_t yl, uint8_t slope)
    {
        int16_t quad = 0;
        switch (slope)
        {
            case SurfaceSlope::CornerUp::north:
                quad = xl + yl - kTileSize;
                break;
            case SurfaceSlope::CornerUp::east:
                quad = xl - yl;
                break;
            case SurfaceSlope::CornerUp::south:
                quad = kTileSize - yl - xl;
                break;
            case SurfaceSlope::CornerUp::west:
                quad = yl - xl;
                break;
        }
        // If the element is in the quadrant with the slope, raise its height
        if (quad > 0)
        {
            return quad / 2;
        }
        return 0;
    }

    static int16_t getOneSideUpLandHeight(int8_t xl, int8_t yl, uint8_t slope)
    {
        int16_t edge = 0;
        switch (slope)
        {
            case SurfaceSlope::SideUp::northeast:
                edge = xl / 2 + 1;
                break;
            case SurfaceSlope::SideUp::southeast:
                edge = (kTileSize - yl) / 2;
                break;
            case SurfaceSlope::SideUp::northwest:
                edge = yl / 2 + 1;
                break;
            case SurfaceSlope::SideUp::southwest:
                edge = (kTileSize - xl) / 2;
                break;
        }
        return edge;
    }

    // This also takes care of the one corner down and one opposite corner up
    static int16_t getOneCornerDownLandHeight(int8_t xl, int8_t yl, uint8_t slope, bool isDoubleHeight)
    {
        int16_t quadExtra = 0;
        int16_t quad = 0;

        switch (slope)
        {
            case SurfaceSlope::CornerDown::west:
                quadExtra = xl + kTileSize - yl;
                quad = xl - yl;
                break;
            case SurfaceSlope::CornerDown::south:
                quadExtra = xl + yl;
                quad = xl + yl - kTileSize - 1;
                break;
            case SurfaceSlope::CornerDown::east:
                quadExtra = kTileSize - xl + yl;
                quad = yl - xl;
                break;
            case SurfaceSlope::CornerDown::north:
                quadExtra = (kTileSize - xl) + (kTileSize - yl);
                quad = kTileSize - yl - xl - 1;
                break;
        }

        if (isDoubleHeight)
        {
            return quadExtra / 2 + 1;
        }
        else
        {
            // This tile is essentially at the next height level
            // so we move *down* the slope
            return quad / 2 + 16;
        }
    }

    static int16_t getValleyLandHeight(int8_t xl, int8_t yl, uint8_t slope)
    {
        int16_t quad = 0;
        switch (slope)
        {
            case SurfaceSlope::Valley::westeast:
                if (xl + yl > kTileSize + 1)
                {
                    quad = kTileSize - xl - yl;
                }
                break;
            case SurfaceSlope::Valley::northsouth:
                quad = xl - yl;
                break;
        }

        if (quad > 0)
        {
            return quad / 2;
        }
        return 0;
    }

    /**
     * Return the absolute height of an element, given its (x, y) coordinates
     * remember to & with 0xFFFF if you don't want water affecting results
     *
     * @param x @<ax>
     * @param y @<cx>
     * @return height @<edx>
     *
     * 0x00467297 rct2: 0x00662783 (numbers different)
     */
    TileHeight getHeight(const Pos2& pos)
    {
        TileHeight height{ 16, 0 };
        // Off the map
        if ((unsigned)pos.x >= (Map::kMapWidth - 1) || (unsigned)pos.y >= (Map::kMapHeight - 1))
            return height;

        auto tile = TileManager::get(pos);
        // Get the surface element for the tile
        auto surfaceEl = tile.surface();

        if (surfaceEl == nullptr)
        {
            return height;
        }

        height.waterHeight = surfaceEl->waterHeight();
        height.landHeight = surfaceEl->baseHeight();

        const auto slope = surfaceEl->slopeCorners();

        // Subtile coords
        const auto xl = pos.x & 0x1f;
        const auto yl = pos.y & 0x1f;

        // Slope logic:
        // Each of the four bits in slope represents that corner being raised
        // slope == 15 (all four bits) is not used and slope == 0 is flat
        // If the extra_height bit is set, then the slope goes up two z-levels (this happens with one corner down with oppisite corner up)

        // We arbitrarily take the SW corner to be closest to the viewer

        switch (slope)
        {
            case SurfaceSlope::flat:
                // Flat surface requires no further calculations.
                break;

            case SurfaceSlope::CornerUp::north:
            case SurfaceSlope::CornerUp::east:
            case SurfaceSlope::CornerUp::south:
            case SurfaceSlope::CornerUp::west:
                height.landHeight += getOneCornerUpLandHeight(xl, yl, slope);
                break;

            case SurfaceSlope::SideUp::northeast:
            case SurfaceSlope::SideUp::southeast:
            case SurfaceSlope::SideUp::northwest:
            case SurfaceSlope::SideUp::southwest:
                height.landHeight += getOneSideUpLandHeight(xl, yl, slope);
                break;

            case SurfaceSlope::CornerDown::north:
            case SurfaceSlope::CornerDown::east:
            case SurfaceSlope::CornerDown::south:
            case SurfaceSlope::CornerDown::west:
                height.landHeight += getOneCornerDownLandHeight(xl, yl, slope, surfaceEl->isSlopeDoubleHeight());
                break;

            case SurfaceSlope::Valley::northsouth:
            case SurfaceSlope::Valley::westeast:
                height.landHeight += getValleyLandHeight(xl, yl, slope);
                break;
        }
        return height;
    }

    static void clearTilePointers()
    {
        std::fill(_tiles.begin(), _tiles.end(), InvalidTile);
    }

    static void set(TilePos2 pos, TileElement* elements)
    {
        _tiles[(pos.y * kMapPitch) + pos.x] = elements;
    }

    // 0x00461348
    void updateTilePointers()
    {
        clearTilePointers();

        TileElement* el = _elements;
        for (tile_coord_t y = 0; y < kMapRows; y++)
        {
            for (tile_coord_t x = 0; x < kMapColumns; x++)
            {
                set(TilePos2(x, y), el);

                // Skip remaining elements on this tile
                do
                {
                    el++;
                } while (!(el - 1)->isLast());
            }
        }

        _elementsEnd = el;
    }

    // 0x0046148F
    void reorganise()
    {
        Ui::setCursor(Ui::CursorId::busy);

        try
        {
            // Allocate a temporary buffer and tighly pack all the tile elements in the map
            std::vector<TileElement> tempBuffer;
            tempBuffer.resize(maxElements * sizeof(TileElement));

            size_t numElements = 0;
            for (tile_coord_t y = 0; y < kMapRows; y++)
            {
                for (tile_coord_t x = 0; x < kMapColumns; x++)
                {
                    auto tile = get(TilePos2(x, y));
                    for (const auto& element : tile)
                    {
                        tempBuffer[numElements] = element;
                        numElements++;
                    }
                }
            }

            // Copy organised elements back to original element buffer
            std::memcpy(_elements, tempBuffer.data(), numElements * sizeof(TileElement));

            // Zero all unused elements
            auto remainingElements = maxElements - numElements;
            std::memset(_elements + numElements, 0, remainingElements * sizeof(TileElement));

            updateTilePointers();

            // Note: original implementation did not revert the cursor
            Ui::setCursor(Ui::CursorId::pointer);
        }
        catch (const std::bad_alloc&)
        {
            Ui::showMessageBox("Bad Alloc", "Bad memory allocation, exiting");
            exitWithError(4370, StringIds::null);
            return;
        }
    }

    // 0x00461393
    bool checkFreeElementsAndReorganise()
    {
        return !(call(0x00461393) & Interop::X86_FLAG_CARRY);
    }

    // 0x00462926
    bool canConstructAt(const Map::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.dl = baseZ;
        regs.dh = clearZ;
        regs.bl = qt.getBaseQuarterOccupied() | (qt.getZQuarterOccupied() << 4);
        return !(call(0x00462926, regs) & Interop::X86_FLAG_CARRY);
    }

    // TODO: Return std::optional
    uint16_t setMapSelectionTiles(const Map::Pos2& loc, const uint8_t selectionType)
    {
        uint16_t xPos = loc.x;
        uint16_t yPos = loc.y;
        uint8_t count = 0;

        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
        {
            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
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

    uint16_t setMapSelectionSingleTile(const Map::Pos2& loc, bool setQuadrant)
    {
        uint16_t xPos = loc.x & 0xFFE0;
        uint16_t yPos = loc.y & 0xFFE0;
        uint16_t cursorQuadrant = Ui::ViewportInteraction::getQuadrantOrCentreFromPos(loc);

        auto count = 0;
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
        {
            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            count++;
        }

        if (setQuadrant && _word_F2448E != cursorQuadrant)
        {
            _word_F2448E = cursorQuadrant;
            count++;
        }
        else if (!setQuadrant && _word_F2448E != 4)
        {
            _word_F2448E = 4;
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
        if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
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
    void mapInvalidateTileFull(Map::Pos2 pos)
    {
        Ui::ViewportManager::invalidate(pos, 0, 1120, ZoomLevel::eighth);
    }

    // 0x0046112C
    void mapInvalidateMapSelectionTiles()
    {
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enableConstruct))
            return;

        for (uint16_t index = 0; index < mapSelectedTilesSize; ++index)
        {
            auto& position = _mapSelectedTiles[index];
            if (position.x == -1)
                break;
            mapInvalidateTileFull(position);
        }
    }

    // 0x0046A747
    void resetSurfaceClearance()
    {
        for (coord_t y = 0; y < kMapHeight; y += kTileSize)
        {
            for (coord_t x = 0; x < kMapWidth; x += kTileSize)
            {
                auto tile = get(x, y);
                auto surface = tile.surface();
                if (surface != nullptr && surface->slope() == 0)
                {
                    surface->setClearZ(surface->baseZ());
                }
            }
        }
    }

    // 0x00469A81
    int16_t mountainHeight(const Map::Pos2& loc)
    {
        // Works out roughly the height of a mountain of area 11 * 11
        // (Its just the heighest point - the lowest point)
        int16_t lowest = std::numeric_limits<int16_t>::max();
        int16_t highest = 0;
        Map::TilePosRangeView range{
            loc - Map::TilePos2{ 5, 5 }, loc + Map::TilePos2{ 5, 5 }
        };
        for (auto& tilePos : range)
        {
            if (!Map::validCoords(tilePos))
            {
                continue;
            }
            auto tile = Map::TileManager::get(tilePos);
            auto* surface = tile.surface();
            auto height = surface->baseHeight();
            lowest = std::min(lowest, height);
            if (surface->slope())
            {
                height += 16;
                if (surface->isSlopeDoubleHeight())
                {
                    height += 16;
                }
            }
            highest = std::max(highest, height);
        }
        return highest - lowest;
    }

    // 0x004C5596
    uint16_t countSurroundingWaterTiles(const Pos2& pos)
    {
        // Search a 10x10 area centred at pos.
        // Initial tile position is the top left of the area.
        auto initialTilePos = Map::TilePos2(pos) - Map::TilePos2(5, 5);

        uint16_t surroundingWaterTiles = 0;
        for (uint8_t yOffset = 0; yOffset < 11; yOffset++)
        {
            for (uint8_t xOffset = 0; xOffset < 11; xOffset++)
            {
                auto tilePos = initialTilePos + Map::TilePos2(xOffset, yOffset);
                if (!Map::validCoords(tilePos))
                    continue;

                auto tile = get(tilePos);
                auto* surface = tile.surface();
                if (surface != nullptr && surface->water() > 0)
                    surroundingWaterTiles++;
            }
        }

        return surroundingWaterTiles;
    }

    // 0x00469B1D
    uint16_t countSurroundingDesertTiles(const Pos2& pos)
    {
        // Search a 10x10 area centred at pos.
        // Initial tile position is the top left of the area.
        auto initialTilePos = Map::TilePos2(pos) - Map::TilePos2(5, 5);

        uint16_t surroundingDesertTiles = 0;

        for (const auto& tilePos : TilePosRangeView(initialTilePos, initialTilePos + Map::TilePos2{ 10, 10 }))
        {
            if (!Map::validCoords(tilePos))
                continue;

            auto tile = get(tilePos);
            auto* surface = tile.surface();
            // Desert tiles can't have water! Oasis aren't deserts.
            if (surface == nullptr || surface->water() != 0)
            {
                continue;
            }
            auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
            if (landObj == nullptr)
            {
                continue;
            }
            if (landObj->hasFlags(LandObjectFlags::isDesert))
            {
                surroundingDesertTiles++;
            }
        }

        return surroundingDesertTiles;
    }

    // 0x004BE048
    uint16_t countSurroundingTrees(const Pos2& pos)
    {
        // Search a 10x10 area centred at pos.
        // Initial tile position is the top left of the area.
        auto initialTilePos = Map::TilePos2(pos) - Map::TilePos2(5, 5);

        uint16_t surroundingTrees = 0;
        for (uint8_t yOffset = 0; yOffset < 11; yOffset++)
        {
            for (uint8_t xOffset = 0; xOffset < 11; xOffset++)
            {
                auto tilePos = initialTilePos + Map::TilePos2(xOffset, yOffset);
                if (!Map::validCoords(tilePos))
                    continue;

                auto tile = get(tilePos);
                for (auto& element : tile)
                {
                    // NB: vanilla was checking for trees above the surface element.
                    // This has been omitted from our implementation.
                    auto* tree = element.as<TreeElement>();
                    if (tree == nullptr)
                        continue;

                    if (tree->isGhost())
                        continue;

                    surroundingTrees++;
                }
            }
        }

        return surroundingTrees;
    }

    static bool update(TileElement& el, const Map::Pos2& loc)
    {
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.esi = X86Pointer(&el);
        regs.bl = el.data()[0] & 0x3F;

        switch (el.type())
        {
            case ElementType::surface:
                call(0x004691FA, regs);
                break;
            case ElementType::building:
            {
                auto& elBuilding = el.get<BuildingElement>();
                return elBuilding.update(loc);
            }
            case ElementType::tree:
                call(0x004BD52B, regs);
                break;
            case ElementType::road:
            {
                auto& elRoad = el.get<RoadElement>();
                return elRoad.update(loc);
            }
            case ElementType::industry:
            {
                auto& elIndustry = el.get<IndustryElement>();
                return elIndustry.update(loc);
            }
            case ElementType::track: break;
            case ElementType::station: break;
            case ElementType::signal: break;
            case ElementType::wall: break;
        }
        return regs.esi != 0;
    }

    // 0x00463ABA
    void update()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            return;
        }

        CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
        auto pos = *_startUpdateLocation;
        for (; pos.y < Map::kMapHeight; pos.y += 16 * Map::kTileSize)
        {
            for (; pos.x < Map::kMapWidth; pos.x += 16 * Map::kTileSize)
            {
                auto tile = TileManager::get(pos);
                for (auto& el : tile)
                {
                    if (el.isGhost())
                        continue;

                    // If update removed/added tiles we must stop loop as pointer is invalid
                    if (!update(el, pos))
                    {
                        break;
                    }
                }
            }
            pos.x -= Map::kMapWidth;
        }
        pos.y -= Map::kMapHeight;

        const TilePos2 tilePos(pos);
        const uint8_t shift = (tilePos.y << 4) + tilePos.x + 9;
        _startUpdateLocation = TilePos2(shift & 0xF, shift >> 4);
        if (shift == 0)
        {
            IndustryManager::updateProducedCargoStats();
        }
    }

    // 0x0048B0C7
    void createDestructExplosion(const Map::Pos3& pos)
    {
        ExplosionSmoke::create(pos + Map::Pos3{ 0, 0, 13 });
        const auto randFreq = gPrng().randNext(20'003, 24'098);
        Audio::playSound(Audio::SoundId::demolishBuilding, pos, -1400, randFreq);
    }

    // 0x0042D8FF
    void removeBuildingElement(BuildingElement& elBuilding, const Map::Pos2& pos)
    {
        if (!elBuilding.isGhost() && !elBuilding.isFlag5())
        {
            if (CompanyManager::getUpdatingCompanyId() != CompanyId::neutral)
            {
                createDestructExplosion(Map::Pos3(pos.x + 16, pos.y + 16, elBuilding.baseHeight()));
            }
        }

        if (elBuilding.multiTileIndex() == 0)
        {
            if (!elBuilding.isGhost())
            {
                auto* buildingObj = elBuilding.getObject();
                if (buildingObj != nullptr)
                {
                    if (!buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
                    {
                        auto buildingCapacity = -buildingObj->producedQuantity[0];
                        auto removedPopulation = buildingCapacity;
                        if (!elBuilding.isConstructed())
                        {
                            removedPopulation = 0;
                        }
                        auto ratingReduction = buildingObj->demolishRatingReduction;
                        auto* town = TownManager::sub_497DC1(pos, removedPopulation, buildingCapacity, ratingReduction, -1);
                        if (town != nullptr)
                        {
                            if (buildingObj->var_AC != 0xFF)
                            {
                                town->var_150[buildingObj->var_AC] -= 1;
                            }
                        }
                    }
                }
            }
        }
        Ui::ViewportManager::invalidate(pos, elBuilding.baseHeight(), elBuilding.clearHeight(), ZoomLevel::eighth);
        TileManager::removeElement(*reinterpret_cast<TileElement*>(&elBuilding));
    }

    // 0x004C482B
    void removeAllWallsOnTile(const Map::TilePos2& pos, SmallZ baseZ)
    {
        std::vector<Map::TileElement*> toDelete;
        auto tile = get(pos);
        for (auto& el : tile)
        {
            auto* elWall = el.as<WallElement>();
            if (elWall == nullptr)
            {
                continue;
            }
            if (baseZ >= elWall->clearZ())
            {
                continue;
            }
            if (baseZ + 12 < elWall->baseZ())
            {
                continue;
            }
            toDelete.push_back(&el);
        }
        // Remove in reverse order to prevent pointer invalidation
        std::for_each(std::rbegin(toDelete), std::rend(toDelete), [&pos](Map::TileElement* el) {
            Ui::ViewportManager::invalidate(pos, el->baseHeight(), el->baseHeight() + 72, ZoomLevel::half);
            removeElement(*el);
        });
    }

    // 0x0047AB9B
    void updateYearly()
    {
        call(0x0047AB9B);
    }

    void registerHooks()
    {
        // This hook can be removed once sub_4599B3 has been implemented
        registerHook(
            0x004BE048,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                regs.dx = countSurroundingTrees({ regs.ax, regs.cx });
                return 0;
            });

        registerHook(
            0x004C5596,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                regs.dx = countSurroundingWaterTiles({ regs.ax, regs.cx });
                return 0;
            });

        registerHook(
            0x0046902E,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                removeSurfaceIndustry({ regs.ax, regs.cx });
                return 0;
            });
    }
}
