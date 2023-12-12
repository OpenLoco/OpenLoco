#include "TileManager.h"
#include "Audio/Audio.h"
#include "BuildingElement.h"
#include "Economy/Economy.h"
#include "Effects/Effect.h"
#include "Effects/ExplosionSmokeEffect.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "IndustryElement.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/BridgeObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TreeObject.h"
#include "Objects/WaterObject.h"
#include "OpenLoco.h"
#include "Random.h"
#include "RoadElement.h"
#include "SceneManager.h"
#include "SignalElement.h"
#include "StationElement.h"
#include "SurfaceElement.h"
#include "TileClearance.h"
#include "TrackElement.h"
#include "TreeElement.h"
#include "Ui.h"
#include "Ui/ViewportInteraction.h"
#include "ViewportManager.h"
#include "WallElement.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <set>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::TileManager
{
    static loco_global<TileElement*, 0x005230C8> _elements;
    static loco_global<TileElement* [0x30004], 0x00E40134> _tiles;
    static loco_global<TileElement*, 0x00F00134> _elementsEnd;
    static loco_global<const TileElement*, 0x00F00158> _F00158;
    static loco_global<uint32_t, 0x00F00168> _F00168;
    static loco_global<coord_t, 0x00F24486> _mapSelectionAX;
    static loco_global<coord_t, 0x00F24488> _mapSelectionBX;
    static loco_global<coord_t, 0x00F2448A> _mapSelectionAY;
    static loco_global<coord_t, 0x00F2448C> _mapSelectionBY;
    static loco_global<uint16_t, 0x00F2448E> _word_F2448E;
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;

    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<Pos2[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;

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

    // 0x004BF476
    void allocateMapElements()
    {
        TileElement* elements = reinterpret_cast<TileElement*>(malloc(0x360000));
        if (elements == nullptr)
        {
            exitWithError(StringIds::game_init_failure, StringIds::unable_to_allocate_enough_memory);
            return;
        }

        _elements = elements;
    }

    // 0x00461179
    void initialise()
    {
        _F00168 = 0;
        getGameState().tileUpdateStartLocation = World::Pos2(0, 0);
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

    std::span<TileElement> getElements()
    {
        return std::span<TileElement>(static_cast<TileElement*>(_elements), getElementsEnd());
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

    void setElements(std::span<TileElement> elements)
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
                *_F00158 = kInvalidTile;
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

    void setRemoveElementPointerChecker(TileElement& element)
    {
        *_F00158 = &element;
    }

    bool wasRemoveOnLastElement()
    {
        return *_F00158 == kInvalidTile;
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

    // 0x00461578
    TileElement* insertElementAfterNoReorg(TileElement* after, ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.bl = baseZ;
        regs.bh = occupiedQuads;
        regs.esi = X86Pointer(after);
        call(0x00461578, regs);
        TileElement* el = X86Pointer<TileElement>(regs.esi);
        el->setType(type);
        return el;
    }

    TileElement** getElementIndex()
    {
        return _tiles.get();
    }

    static constexpr size_t getTileIndex(const TilePos2& pos)
    {
        // This is the same as (y * kMapPitch) + x
        return (pos.y << 9) | pos.x;
    }

    Tile get(TilePos2 pos)
    {
        const auto index = getTileIndex(pos);
        if (index >= _tiles.size())
        {
            Logging::error("Attempted to get tile out of bounds! ({0}, {1})", pos.x, pos.y);
            return Tile(pos, nullptr);
        }

        auto data = _tiles[index];
        if (data == kInvalidTile)
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
        return get(TilePos2(x / World::kTileSize, y / World::kTileSize));
    }

    static void set(TilePos2 pos, TileElement* elements)
    {
        const auto index = getTileIndex(pos);
        _tiles[index] = elements;
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
        if ((unsigned)pos.x >= (World::kMapWidth - 1) || (unsigned)pos.y >= (World::kMapHeight - 1))
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

    static uint8_t getCornerDownMask(const uint8_t cornerUp)
    {
        switch (cornerUp)
        {
            case SurfaceSlope::CornerUp::north:
                return SurfaceSlope::CornerDown::south;

            case SurfaceSlope::CornerUp::south:
                return SurfaceSlope::CornerDown::north;

            case SurfaceSlope::CornerUp::west:
                return SurfaceSlope::CornerDown::east;

            case SurfaceSlope::CornerUp::east:
                return SurfaceSlope::CornerDown::west;
        }
        return SurfaceSlope::flat;
    }

    // TODO: rename this function
    SmallZ getSurfaceCornerHeight(const SurfaceElement& surface, const uint8_t cornerUp)
    {
        auto baseZ = surface.baseZ();
        if (surface.slope() & cornerUp)
        {
            baseZ += kSmallZStep;
            uint8_t cornerDownMask = getCornerDownMask(cornerUp);
            if (surface.isSlopeDoubleHeight() && surface.slope() == cornerDownMask)
            {
                baseZ += kSmallZStep;
            }
        }

        return baseZ;
    }

    SmallZ getSurfaceCornerHeight(const SurfaceElement& surface)
    {
        auto baseZ = surface.baseZ();
        if (surface.slope())
        {
            baseZ += kSmallZStep;
            if (surface.isSlopeDoubleHeight())
            {
                baseZ += kSmallZStep;
            }
        }

        return baseZ;
    }

    static void clearTilePointers()
    {
        std::fill(_tiles.begin(), _tiles.end(), const_cast<TileElement*>(kInvalidTile));
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
        const auto curCursor = Ui::getCursor();
        Ui::setCursor(Ui::CursorId::busy);

        try
        {
            // Allocate a temporary buffer and tightly pack all the tile elements in the map
            std::vector<TileElement> tempBuffer;
            tempBuffer.resize(maxElements);

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
        }
        catch (const std::bad_alloc&)
        {
            Ui::showMessageBox("Bad Alloc", "Bad memory allocation, exiting");
            exitWithError(4370, StringIds::null);
        }

        // Note: original implementation did not revert the cursor
        Ui::setCursor(curCursor);
    }

    // 0x00461393
    bool checkFreeElementsAndReorganise()
    {
        return !(call(0x00461393) & Interop::X86_FLAG_CARRY);
    }

    CompanyId getTileOwner(const World::TileElement& el)
    {
        CompanyId owner = CompanyId::null;

        if (auto* elTrack = el.as<TrackElement>();
            elTrack != nullptr)
        {
            owner = elTrack->owner();
        }
        else if (auto* elRoad = el.as<RoadElement>();
                 elRoad != nullptr)
        {
            owner = elRoad->owner();
        }
        else if (auto* elStation = el.as<StationElement>();
                 elStation != nullptr)
        {
            if (elStation->stationType() == StationType::trainStation)
            {
                if (auto* prevElTrack = el.prev()->as<TrackElement>();
                    prevElTrack != nullptr)
                {
                    owner = prevElTrack->owner();
                }
            }
            else if (elStation->stationType() == StationType::roadStation)
            {
                if (auto* prevElRoad = el.prev()->as<RoadElement>();
                    prevElRoad != nullptr)
                {
                    owner = prevElRoad->owner();
                }
            }
            else
            {
                owner = elStation->owner();
            }
        }
        else if (auto* elSignal = el.as<SignalElement>();
                 elSignal != nullptr)
        {
            if (auto* prevElTrack = el.prev()->as<TrackElement>();
                prevElTrack != nullptr)
            {
                owner = prevElTrack->owner();
            }
        }
        return owner;
    }

    // TODO: Return std::optional
    uint16_t setMapSelectionTiles(const World::Pos2& loc, const uint8_t selectionType)
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

    uint16_t setMapSelectionSingleTile(const World::Pos2& loc, bool setQuadrant)
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
    void mapInvalidateTileFull(World::Pos2 pos)
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
    int16_t mountainHeight(const World::Pos2& loc)
    {
        // Works out roughly the height of a mountain of area 11 * 11
        // (Its just the heighest point - the lowest point)
        int16_t lowest = std::numeric_limits<int16_t>::max();
        int16_t highest = 0;
        const auto initialTilePos = toTileSpace(loc);
        auto range = getClampedRange(initialTilePos - TilePos2{ 5, 5 }, initialTilePos + TilePos2{ 5, 5 });
        for (auto& tilePos : range)
        {
            auto tile = World::TileManager::get(tilePos);
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
        auto initialTilePos = World::toTileSpace(pos) - World::TilePos2(5, 5);

        uint16_t surroundingWaterTiles = 0;
        for (uint8_t yOffset = 0; yOffset < 11; yOffset++)
        {
            for (uint8_t xOffset = 0; xOffset < 11; xOffset++)
            {
                auto tilePos = initialTilePos + World::TilePos2(xOffset, yOffset);
                if (!World::validCoords(tilePos))
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
        auto initialTilePos = toTileSpace(pos) - TilePos2(5, 5);

        uint16_t surroundingDesertTiles = 0;

        for (const auto& tilePos : getClampedRange(initialTilePos, initialTilePos + TilePos2{ 10, 10 }))
        {
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
        auto initialTilePos = World::toTileSpace(pos) - World::TilePos2(5, 5);

        uint16_t surroundingTrees = 0;
        for (uint8_t yOffset = 0; yOffset < 11; yOffset++)
        {
            for (uint8_t xOffset = 0; xOffset < 11; xOffset++)
            {
                auto tilePos = initialTilePos + World::TilePos2(xOffset, yOffset);
                if (!World::validCoords(tilePos))
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

    // 0x004C5604
    uint16_t countNearbyWaterTiles(Pos2 pos)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004C5604, regs);
        return regs.dx;
    }

    static bool update(TileElement& el, const World::Pos2& loc)
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
        auto pos = getGameState().tileUpdateStartLocation;
        for (; pos.y < World::kMapHeight; pos.y += 16 * World::kTileSize)
        {
            for (; pos.x < World::kMapWidth; pos.x += 16 * World::kTileSize)
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
            pos.x -= World::kMapWidth;
        }
        pos.y -= World::kMapHeight;

        const auto tilePos = World::toTileSpace(pos);
        const uint8_t shift = (tilePos.y << 4) + tilePos.x + 9;
        getGameState().tileUpdateStartLocation = World::toWorldSpace(TilePos2(shift & 0xF, shift >> 4));
        if (shift == 0)
        {
            IndustryManager::updateProducedCargoStats();
        }
    }

    // 0x0048B089
    static void playDemolishTreeSound(const World::Pos3& loc)
    {
        const auto frequency = gPrng2().randNext(20003, 24098);
        Audio::playSound(Audio::SoundId::demolishTree, loc, -1100, frequency);
    }

    // 0x004BB432
    // bl = flags;
    // esi = X86Pointer(&element);
    // ax = pos.x;
    // cx = pos.y;
    void removeTree(World::TreeElement& element, const uint8_t flags, const World::Pos2& pos)
    {
        if ((!element.isGhost() && !element.isAiAllocated())
            && GameCommands::getUpdatingCompanyId() != CompanyId::neutral)
        {
            auto loc = World::Pos3(pos.x, pos.y, element.baseHeight());
            playDemolishTreeSound(loc);
        }

        if ((flags & GameCommands::Flags::ghost) == 0)
        {
            auto treeObj = ObjectManager::get<TreeObject>(element.treeObjectId());
            auto ratingReduction = treeObj->demolishRatingReduction;
            TownManager::updateTownInfo(pos, 0, 0, ratingReduction, 0);
        }

        auto zMin = element.baseHeight();
        auto zMax = element.clearHeight();
        Ui::ViewportManager::invalidate(pos, zMin, zMax, ZoomLevel::eighth, 56);

        World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(&element));
    }

    // 0x0048B0C7
    void createDestructExplosion(const World::Pos3& pos)
    {
        ExplosionSmoke::create(pos + World::Pos3{ 0, 0, 13 });
        const auto randFreq = gPrng1().randNext(20'003, 24'098);
        Audio::playSound(Audio::SoundId::demolishBuilding, pos, -1400, randFreq);
    }

    // 0x0042D8FF
    void removeBuildingElement(BuildingElement& elBuilding, const World::Pos2& pos)
    {
        if (!elBuilding.isGhost() && !elBuilding.isAiAllocated())
        {
            if (CompanyManager::getUpdatingCompanyId() != CompanyId::neutral)
            {
                createDestructExplosion(World::Pos3(pos.x + 16, pos.y + 16, elBuilding.baseHeight()));
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
                        auto* town = TownManager::updateTownInfo(pos, removedPopulation, buildingCapacity, ratingReduction, -1);
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
    void removeAllWallsOnTileAbove(const World::TilePos2& pos, SmallZ baseZ)
    {
        std::vector<World::TileElement*> toDelete;
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
        std::for_each(std::rbegin(toDelete), std::rend(toDelete), [&pos](World::TileElement* el) {
            Ui::ViewportManager::invalidate(World::toWorldSpace(pos), el->baseHeight(), el->baseHeight() + 72, ZoomLevel::half);
            removeElement(*el);
        });
    }

    // 0x004C4979
    void removeAllWallsOnTileBelow(const World::TilePos2& pos, SmallZ baseZ)
    {
        std::vector<World::TileElement*> toDelete;
        auto tile = get(pos);
        for (auto& el : tile)
        {
            auto* elWall = el.as<WallElement>();
            if (elWall == nullptr)
            {
                continue;
            }
            if (baseZ < elWall->clearZ() && baseZ + 12 <= elWall->baseZ())
            {
                continue;
            }
            toDelete.push_back(&el);
        }
        // Remove in reverse order to prevent pointer invalidation
        std::for_each(std::rbegin(toDelete), std::rend(toDelete), [&pos](World::TileElement* el) {
            Ui::ViewportManager::invalidate(World::toWorldSpace(pos), el->baseHeight(), el->baseHeight() + 72, ZoomLevel::half);
            removeElement(*el);
        });
    }

    // 0x004690FC
    void setTerrainStyleAsCleared(const Pos2& pos)
    {
        auto* surface = World::TileManager::get(pos).surface();
        if (surface == nullptr)
        {
            return;
        }
        if (surface->isIndustrial())
        {
            return;
        }
        if (surface->var_6_SLR5() > 0)
        {
            surface->setVar6SLR5(0);
            surface->setVar4SLR5(0);

            Ui::ViewportManager::invalidate(pos, surface->baseHeight(), surface->baseHeight() + 32, ZoomLevel::eighth);
        }
        if (surface->var_4_E0() > 0)
        {
            surface->setVar4SLR5(0);

            Ui::ViewportManager::invalidate(pos, surface->baseHeight(), surface->baseHeight() + 32, ZoomLevel::eighth);
        }
    }

    // 0x00468651
    uint32_t adjustSurfaceHeight(World::Pos2 pos, SmallZ targetBaseZ, uint8_t slopeFlags, std::set<World::Pos3, LessThanPos3>& removedBuildings, uint8_t flags)
    {
        if (!validCoords(pos))
        {
            GameCommands::setErrorText(StringIds::off_edge_of_map);
            return GameCommands::FAILURE;
        }

        if (targetBaseZ < 4)
        {
            GameCommands::setErrorText(StringIds::error_too_low);
            return GameCommands::FAILURE;
        }
        if (targetBaseZ > 160
            || (targetBaseZ == 160 && (slopeFlags & 0x1F) != 0)
            || (targetBaseZ == 156 && (slopeFlags & 0x10) != 0))
        {
            GameCommands::setErrorText(StringIds::error_too_high);
            return GameCommands::FAILURE;
        }

        currency32_t totalCost = 0;

        if (flags & GameCommands::Flags::apply)
        {
            removeSurfaceIndustry(pos);

            if (!isEditorMode())
            {
                setTerrainStyleAsCleared(pos);
            }

            auto clearHeight = getHeight(pos).landHeight;
            removeAllWallsOnTileAbove(toTileSpace(pos), clearHeight / 4);
        }

        // Compute cost of landscape operation
        auto tile = get(pos);
        auto* surface = tile.surface();
        auto landObj = ObjectManager::get<LandObject>(surface->terrain());
        totalCost += Economy::getInflationAdjustedCost(landObj->costFactor, landObj->costIndex, 10);

        auto targetClearZ = targetBaseZ;
        if (slopeFlags & 0x1F)
        {
            targetClearZ += kSmallZStep;
        }
        if (slopeFlags & SurfaceSlope::doubleHeight)
        {
            targetClearZ += kSmallZStep;
        }

        // If surface is in use as a water(route), and has water, ensure we don't remove it
        if (surface->hasType6Flag() && surface->water() > 0)
        {
            auto waterHeight = (surface->water() - 1) * kMicroToSmallZStep;

            if (waterHeight < targetClearZ)
            {
                GameCommands::setErrorText(StringIds::water_channel_currently_needed_by_ships);
                return GameCommands::FAILURE;
            }
        }

        // Bind our local vars to the tile clear function
        auto clearFunc = [pos, &removedBuildings, flags, &totalCost](TileElement& el) {
            return TileClearance::clearWithDefaultCollision(el, pos, removedBuildings, flags, totalCost);
        };

        QuarterTile qt(0xF, 0);
        if (!TileClearance::applyClearAtAllHeights(pos, targetBaseZ, targetClearZ, qt, clearFunc))
        {
            return GameCommands::FAILURE;
        }

        // Check bridge requirements for track and road elements
        const auto tileIt = get(pos);
        for (auto& el : tileIt)
        {
            if (!(el.type() == ElementType::track || el.type() == ElementType::road))
            {
                continue;
            }

            if (el.isGhost())
            {
                continue;
            }

            auto height = el.baseZ() - targetBaseZ;
            if (height < 0)
            {
                continue;
            }

            if (el.type() == ElementType::track)
            {
                auto* trackEl = el.as<TrackElement>();
                if (trackEl != nullptr)
                {
                    if (trackEl->hasBridge())
                    {
                        auto* bridgeObj = ObjectManager::get<BridgeObject>(trackEl->bridge());
                        if (height > bridgeObj->maxHeight * kMicroToSmallZStep)
                        {
                            GameCommands::setErrorText(StringIds::bridge_already_at_maximum_height);
                            return GameCommands::FAILURE;
                        }
                    }
                    else
                    {
                        auto* trackObj = ObjectManager::get<TrackObject>(trackEl->trackObjectId());
                        FormatArguments args{};
                        args.push(trackObj->name);
                        GameCommands::setErrorText(StringIds::stringid_requires_a_bridge);
                        return GameCommands::FAILURE;
                    }
                }
            }
            else if (el.type() == ElementType::road)
            {
                auto* roadEl = el.as<RoadElement>();
                if (roadEl != nullptr)
                {
                    if (roadEl->hasBridge())
                    {
                        auto* bridgeObj = ObjectManager::get<BridgeObject>(roadEl->bridge());
                        if (height > bridgeObj->maxHeight * kMicroToSmallZStep)
                        {
                            GameCommands::setErrorText(StringIds::bridge_already_at_maximum_height);
                            return GameCommands::FAILURE;
                        }
                    }
                    else
                    {
                        auto* roadObj = ObjectManager::get<RoadObject>(roadEl->roadObjectId());
                        FormatArguments args{};
                        args.push(roadObj->name);
                        GameCommands::setErrorText(StringIds::stringid_requires_a_bridge);
                        return GameCommands::FAILURE;
                    }
                }
            }
        }

        if (!(flags & GameCommands::Flags::apply))
        {
            return totalCost;
        }

        surface = tileIt.surface();
        if (!isEditorMode())
        {
            // Reset terrain growth when not in editor
            surface->setTerrain(surface->terrain());
        }

        surface->setBaseZ(targetBaseZ);
        surface->setClearZ(targetBaseZ);
        surface->setSlope(slopeFlags);

        landObj = ObjectManager::get<LandObject>(surface->terrain());
        if (landObj->hasFlags(LandObjectFlags::unk1) && !isEditorMode())
        {
            surface->setTerrain(landObj->var_07);
        }

        if (surface->water() * kMicroToSmallZStep <= targetBaseZ)
        {
            surface->setWater(0);
        }

        mapInvalidateTileFull(pos);
        return totalCost;
    }

    // 0x004C4C28
    uint32_t adjustWaterHeight(World::Pos2 pos, SmallZ targetHeight, std::set<World::Pos3, LessThanPos3>& removedBuildings, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        GameCommands::setPosition(World::Pos3(pos.x + World::kTileSize / 2, pos.y + World::kTileSize / 2, targetHeight * kMicroToSmallZStep));

        if (targetHeight < 4)
        {
            GameCommands::setErrorText(StringIds::error_too_low);
            return GameCommands::FAILURE;
        }

        if (targetHeight >= 116)
        {
            GameCommands::setErrorText(StringIds::error_too_high);
            return GameCommands::FAILURE;
        }

        currency32_t totalCost = 0;

        if (flags & GameCommands::Flags::apply)
        {
            removeSurfaceIndustry(pos);

            if (!isEditorMode())
            {
                setTerrainStyleAsCleared(pos);
            }

            auto clearHeight = getHeight(pos).landHeight;
            removeAllWallsOnTileAbove(toTileSpace(pos), clearHeight / kSmallZStep);
        }

        auto tile = get(pos);
        auto* surface = tile.surface();

        auto referenceZ = surface->baseZ();
        auto water = surface->water();
        if (water > 0)
        {
            referenceZ = water * kMicroToSmallZStep;
        }

        const auto baseTargetZ = std::min(targetHeight, referenceZ);
        const auto clearTargetZ = std::max(targetHeight, referenceZ);

        // Bind our local vars to the tile clear function
        auto clearFunc = [pos, &removedBuildings, flags, &totalCost](TileElement& el) {
            return TileClearance::clearWithDefaultCollision(el, pos, removedBuildings, flags, totalCost);
        };

        QuarterTile qt(0xF, 0);
        if (!TileClearance::applyClearAtAllHeights(pos, baseTargetZ, clearTargetZ, qt, clearFunc))
        {
            return GameCommands::FAILURE;
        }

        if (surface->hasType6Flag())
        {
            GameCommands::setErrorText(StringIds::water_channel_currently_needed_by_ships);
            return GameCommands::FAILURE;
        }

        auto* waterObj = ObjectManager::get<WaterObject>();
        totalCost += Economy::getInflationAdjustedCost(waterObj->costFactor, waterObj->costIndex, 10);

        if (flags & GameCommands::Flags::apply)
        {
            if (targetHeight <= surface->baseZ())
            {
                surface->setWater(0);
            }
            else
            {
                surface->setWater(targetHeight / kMicroToSmallZStep);
            }
            surface->setType6Flag(false);
            surface->setVar7(0);

            mapInvalidateTileFull(pos);
        }
        return totalCost;
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
