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
#include <set>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::World::TileManager
{
    static std::vector<TileElement> _elements;   // 0x005230C8
    static std::vector<TileElement*> _tiles;     // 0x00E40134
    static ptrdiff_t _elementsEnd = 0;           // 0x00F00134
    static const TileElement* _F00158 = nullptr; // 0x00F00158
    static uint32_t _periodicDefragStartTile;    // 0x00F00168
    static bool _disablePeriodicDefrag;          // 0x0050BF6C

    constexpr size_t kMaxElementsOnOneTile = 1024; // If you exceed this then the game may buffer overflow in certain situations

    static coord_t mapRows = 384;
    static coord_t mapColumns = 384;

    void setMapSize(coord_t cols, coord_t rows)
    {
        mapRows = rows;
        mapColumns = cols;

        // resize map
        allocateMapElements();
        initialise();
    }

    coord_t getMapRows()
    {
        return mapRows;
    }

    coord_t getMapColumns()
    {
        return mapColumns;
    }

    coord_t getMapHeight()
    {
        return getMapRows() * kTileSize;
    }

    coord_t getMapWidth()
    {
        return getMapColumns() * kTileSize;
    }

    uint32_t getMapSize()
    {
        return getMapRows() * getMapColumns();
    }

    int32_t getNumTiles()
    {
        return kMapPitch * getMapColumns();
    }

    size_t getMaxElements()
    {
        return 3 * getMapColumns() * getMapRows();
    }

    size_t getMaxUsableElements()
    {
        return getMaxElements() - kMaxElementsOnOneTile;
    }

    bool validCoord(const coord_t coord)
    {
        return (coord >= 0) && (coord < getMapWidth());
    }

    bool validTileCoord(const tile_coord_t coord)
    {
        return (coord >= 0) && (coord < getMapColumns());
    }

    bool validCoords(const Pos2& coords)
    {
        return validCoord(coords.x) && validCoord(coords.y);
    }

    bool validCoords(const TilePos2& coords)
    {
        return validTileCoord(coords.x) && validTileCoord(coords.y);
    }

    // drawing coordinates validation differs from general valid coordinate validation
    bool drawableCoord(const coord_t coord)
    {
        return (coord >= kTileSize) && (coord < (getMapWidth() - kTileSize - 1));
    }

    bool drawableTileCoord(const tile_coord_t coord)
    {
        return (coord >= 1) && (coord < (World::TileManager::getMapColumns() - 2));
    }

    bool drawableCoords(const Pos2& coords)
    {
        return drawableCoord(coords.x) && drawableCoord(coords.y);
    }

    bool drawableCoords(const TilePos2& coords)
    {
        return drawableTileCoord(coords.x) && drawableTileCoord(coords.y);
    }

    coord_t clampTileCoord(coord_t coord)
    {
        return std::clamp<coord_t>(coord, 0, World::TileManager::getMapColumns() - 1);
    }

    coord_t clampCoord(coord_t coord)
    {
        return std::clamp<coord_t>(coord, 0, getMapWidth() - 1);
    }

    void disablePeriodicDefrag()
    {
        _disablePeriodicDefrag = true;
    }

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

    // 0x0046908D
    void removeSurfaceIndustryAtHeight(const Pos3& pos)
    {
        auto* elSurface = World::TileManager::get(pos).surface();
        if (elSurface != nullptr)
        {
            // If underground
            if (elSurface->baseHeight() > pos.z)
            {
                return;
            }
            // If quite high in the air
            if (pos.z - elSurface->baseHeight() >= 12 * World::kSmallZStep)
            {
                return;
            }

            elSurface->removeIndustry(pos);
        }
    }

    // 0x004BF476
    void allocateMapElements()
    {
        try
        {
            _elements.resize(getMaxElements());
            _tiles.resize(getNumTiles());
        }
        catch (std::bad_alloc&)
        {
            exitWithError(StringIds::game_init_failure, StringIds::unable_to_allocate_enough_memory);
            return;
        }
    }

    // 0x00461179
    void initialise()
    {
        _periodicDefragStartTile = 0;
        getGameState().tileUpdateStartLocation = World::Pos2(0, 0);
        const auto landType = getGameState().lastLandOption == 0xFF ? 0 : getGameState().lastLandOption;

        SurfaceElement defaultElement{};
        defaultElement.setTerrain(landType);
        defaultElement.setBaseZ(4);
        defaultElement.setLastFlag(true);

        for (auto& element : _elements)
        {
            element = *reinterpret_cast<TileElement*>(&defaultElement);
        }
        updateTilePointers();
        getGameState().flags |= GameStateFlags::tileManagerLoaded;
    }

    std::span<TileElement> getElements()
    {
        return std::span<TileElement>(static_cast<TileElement*>(&*_elements.begin()), _elementsEnd);
    }

    uint32_t numFreeElements()
    {
        return getMaxElements() - _elementsEnd;
    }

    void setElements(std::span<TileElement> elements)
    {
        std::ranges::copy(elements, _elements.begin());
        TileManager::updateTilePointers();
    }

    // Note: Must be past the last tile flag
    static void markElementAsFree(TileElement& element)
    {
        element.setBaseZ(255);
        if (std::distance(&*_elements.begin(), element.next()) == _elementsEnd)
        {
            _elementsEnd--;
        }
    }

    // 0x00461760
    void removeElement(TileElement& element)
    {
        // This is used to indicate if the caller can still use this pointer
        if (&element == _F00158)
        {
            if (element.isLast())
            {
                _F00158 = nullptr;
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
            // Move all of the elements up one until last for the tile
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
        _F00158 = &element;
    }

    bool wasRemoveOnLastElement()
    {
        return _F00158 == nullptr;
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

    static std::pair<TileElement*, TileElement*> insertElementPrepareDest(const TilePos2 pos)
    {
        const auto index = getTileIndex(pos);
        if (index >= _tiles.size())
        {
            Logging::error("Attempted to get tile out of bounds! ({0}, {1})", pos.x, pos.y);
            return std::make_pair(nullptr, nullptr);
        }

        auto* source = _tiles[index];
        // _elementsEnd points to the free space at the end of the
        // tile elements. You must always check there is space (checkFreeElementsAndReorganise)
        // prior to calling this function!
        auto* dest = &_elements[_elementsEnd];
        set(pos, dest);
        return std::make_pair(source, dest);
    }

    static TileElement* insertElementEnd(ElementType type, uint8_t baseZ, uint8_t occupiedQuads, TileElement* source, TileElement* dest, bool lastFound)
    {
        auto* newElement = dest++;
        // Clear the element
        *newElement = TileElement{};

        newElement->setType(type);
        newElement->setBaseZ(baseZ);
        newElement->setClearZ(baseZ);
        newElement->setOccupiedQuarter(occupiedQuads);

        if (lastFound)
        {
            newElement->setLastFlag(true);
        }
        else
        {
            // Copy all of the elements that are above the new tile
            do
            {
                *dest = *source;
                source->setBaseZ(0xFFU);
                source++;
            } while (!dest++->isLast());
        }
        _elementsEnd = std::distance(&*_elements.begin(), dest);

        return newElement;
    }

    // 0x004616D6
    TileElement* insertElement(ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads)
    {
        checkFreeElementsAndReorganise();

        auto [source, dest] = insertElementPrepareDest(toTileSpace(pos));
        if (source == nullptr)
        {
            return nullptr;
        }

        bool lastFound = false;
        // Copy all of the elements that are underneath the new tile (or till end)
        while (baseZ >= source->baseZ())
        {
            *dest = *source;
            source->setBaseZ(0xFFU);
            source++;
            if (dest->isLast())
            {
                // The new element will become the last
                // so we are clearing the flag
                dest->setLastFlag(false);
                dest++;
                lastFound = true;
                break;
            }
            dest++;
        }

        return insertElementEnd(type, baseZ, occupiedQuads, source, dest, lastFound);
    }

    // 0x0046166C
    RoadElement* insertElementRoad(const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads)
    {
        checkFreeElementsAndReorganise();

        auto [source, dest] = insertElementPrepareDest(toTileSpace(pos));
        if (source == nullptr)
        {
            return nullptr;
        }

        bool lastFound = false;
        auto isRoadStation = [](const TileElement* source, SmallZ baseZ) {
            if (baseZ != source->baseZ())
            {
                return false;
            }
            auto* srcStation = source->as<StationElement>();
            if (srcStation == nullptr)
            {
                return false;
            }
            return srcStation->stationType() == StationType::roadStation;
        };

        // Copy all of the elements that are underneath the new tile (or till end)
        while (baseZ >= source->baseZ() && !isRoadStation(source, baseZ))
        {
            *dest = *source;
            source->setBaseZ(0xFFU);
            source++;
            if (dest->isLast())
            {
                // The new element will become the last
                // so we are clearing the flag
                dest->setLastFlag(false);
                dest++;
                lastFound = true;
                break;
            }
            dest++;
        }

        return insertElementEnd(ElementType::road, baseZ, occupiedQuads, source, dest, lastFound)->as<RoadElement>();
    }

    // 0x00461578
    TileElement* insertElementAfterNoReorg(TileElement* after, ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads)
    {
        auto [source, dest] = insertElementPrepareDest(toTileSpace(pos));
        if (source == nullptr)
        {
            return nullptr;
        }

        bool lastFound = false;
        // Copy all of the elements that are underneath the new tile (or till end)
        while (source <= after)
        {
            *dest = *source;
            source->setBaseZ(0xFFU);
            source++;
            if (dest->isLast())
            {
                // The new element will become the last
                // so we are clearing the flag
                dest->setLastFlag(false);
                dest++;
                lastFound = true;
                break;
            }
            dest++;
        }

        return insertElementEnd(type, baseZ, occupiedQuads, source, dest, lastFound);
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
        if (pos.x >= (World::TileManager::getMapWidth() - 1) || pos.y >= (World::TileManager::getMapHeight() - 1))
        {
            return height;
        }

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

        // Sub-tile coords
        const auto xl = pos.x & 0x1f;
        const auto yl = pos.y & 0x1f;

        // Slope logic:
        // Each of the four bits in slope represents that corner being raised
        // slope == 15 (all four bits) is not used and slope == 0 is flat
        // If the extra_height bit is set, then the slope goes up two z-levels (this happens with one corner down with opposite corner up)

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

    SmallZ getSurfaceCornerDownHeight(const SurfaceElement& surface, const uint8_t cornerMask)
    {
        auto baseZ = surface.baseZ();
        if (surface.slope() & cornerMask)
        {
            baseZ += kSmallZStep;
            uint8_t cornerDownMask = getCornerDownMask(cornerMask);
            if (surface.isSlopeDoubleHeight() && surface.slopeCorners() == cornerDownMask)
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
        std::ranges::fill(_tiles, nullptr);
    }

    // 0x00461348
    void updateTilePointers()
    {
        clearTilePointers();

        auto el = _elements.begin();
        for (tile_coord_t y = 0; y < World::TileManager::getMapRows(); y++)
        {
            for (tile_coord_t x = 0; x < World::TileManager::getMapColumns(); x++)
            {
                set(TilePos2(x, y), &*el);

                // Skip remaining elements on this tile
                do
                {
                    el++;
                } while (!(el - 1)->isLast());
            }
        }

        _elementsEnd = std::distance(_elements.begin(), el);
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
            tempBuffer.resize(getMaxElements());

            size_t numElements = 0;
            for (tile_coord_t y = 0; y < World::TileManager::getMapRows(); y++)
            {
                for (tile_coord_t x = 0; x < World::TileManager::getMapColumns(); x++)
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
            _elements = tempBuffer;

            updateTilePointers();
        }
        catch (const std::bad_alloc&)
        {
            exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
        }

        // Note: original implementation did not revert the cursor
        Ui::setCursor(curCursor);
    }

    // 0x004613F0
    void defragmentTilePeriodic()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            return;
        }
        if (_disablePeriodicDefrag)
        {
            _disablePeriodicDefrag = false;
            return;
        }
        _disablePeriodicDefrag = false;

        const uint32_t searchStart = _periodicDefragStartTile + 1;
        for (auto i = 0; i < getNumTiles(); ++i)
        {
            const auto j = (i + searchStart) % getNumTiles();
            if (_tiles[j] != nullptr)
            {
                _periodicDefragStartTile = j;
                break;
            }
        }

        auto* firstTile = _tiles[_periodicDefragStartTile];
        auto* emptyTile = firstTile - 1;
        while (emptyTile != &_elements[0] && emptyTile->baseZ() == 0xFFU)
        {
            emptyTile--;
        }
        emptyTile++;
        if (emptyTile == firstTile)
        {
            return;
        }

        _tiles[_periodicDefragStartTile] = emptyTile;
        {
            auto* dest = emptyTile;
            auto* source = firstTile;
            do
            {
                *dest = *source;
                source->setBaseZ(0xFFU);
                source++;
            } while (!dest++->isLast());
        }

        // Its possible we have freed up elements at the end so this
        // looks to see if we should move the element end
        auto newEnd = _elementsEnd - 1;
        while (_elements[newEnd].baseZ() == 0xFFU)
        {
            newEnd--;
        }
        _elementsEnd = newEnd + 1;
    }

    // 0x00461393
    bool checkFreeElementsAndReorganise()
    {
        if (numFreeElements() > kMaxElementsOnOneTile)
        {
            return true;
        }
        // First try a basic defrag multiple times
        for (auto i = 0U; i < 1000; ++i)
        {
            defragmentTilePeriodic();
        }
        if (numFreeElements() > kMaxElementsOnOneTile)
        {
            return true;
        }
        // Now try a full defrag
        reorganise();
        if (numFreeElements() > kMaxElementsOnOneTile)
        {
            return true;
        }
        GameCommands::setErrorText(StringIds::landscape_data_area_full);
        return false;
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

    // 0x004CBE5F
    // regs.ax: pos.x
    // regs.cx: pos.y
    void mapInvalidateTileFull(World::Pos2 pos)
    {
        Ui::ViewportManager::invalidate(pos, 0, 1120, ZoomLevel::eighth);
    }

    // 0x0046A747
    void resetSurfaceClearance()
    {
        for (coord_t y = 0; y < getMapHeight(); y += kTileSize)
        {
            for (coord_t x = 0; x < getMapWidth(); x += kTileSize)
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
        // (Its just the highest point - the lowest point)
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
        for (const auto& tilePos : getClampedRange(initialTilePos, initialTilePos + TilePos2{ 10, 10 }))
        {
            auto tile = get(tilePos);
            auto* surface = tile.surface();
            if (surface != nullptr && surface->water() > 0)
            {
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
        for (const auto& tilePos : getClampedRange(initialTilePos, initialTilePos + TilePos2{ 10, 10 }))
        {
            auto tile = get(tilePos);
            for (auto& element : tile)
            {
                // NB: vanilla was checking for trees above the surface element.
                // This has been omitted from our implementation.
                auto* tree = element.as<TreeElement>();
                if (tree == nullptr)
                {
                    continue;
                }

                if (tree->isGhost())
                {
                    continue;
                }

                surroundingTrees++;
            }
        }

        return surroundingTrees;
    }

    // 0x004C5604
    uint16_t countNearbyWaterTiles(Pos2 pos)
    {
        // Search a 20x20 area in a checkerboard pattern, centred at pos.
        // Initial tile position is the top left of the area.
        auto initialTilePos = World::toTileSpace(pos) - World::TilePos2(10, 10);

        uint16_t nearbyWaterTiles = 0;
        for (const auto& tilePos : getClampedRange(initialTilePos, initialTilePos + TilePos2{ 20, 20 }))
        {
            // Skip every other tile, depending on initial position
            if ((tilePos.x & 1) != (initialTilePos.x & 1) || (tilePos.y & 1) != (initialTilePos.y & 1))
            {
                continue;
            }

            auto tile = get(tilePos);
            auto* surface = tile.surface();
            if (surface != nullptr && surface->water() > 0)
            {
                nearbyWaterTiles++;
            }
        }

        return nearbyWaterTiles;
    }

    static bool update(TileElement& el, const World::Pos2& loc)
    {
        switch (el.type())
        {
            case ElementType::surface:
            {
                auto& elSurface = el.get<SurfaceElement>();
                return updateSurface(elSurface, loc);
            }
            case ElementType::building:
            {
                auto& elBuilding = el.get<BuildingElement>();
                return elBuilding.update(loc);
            }
            case ElementType::tree:
            {
                auto& elTree = el.get<TreeElement>();
                return updateTreeElement(elTree, loc);
            }
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
        return true;
    }

    // 0x00463ABA
    void update()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            return;
        }

        GameCommands::setUpdatingCompanyId(CompanyId::neutral);
        auto pos = getGameState().tileUpdateStartLocation;
        for (; pos.y < World::TileManager::getMapHeight(); pos.y += 16 * World::kTileSize)
        {
            for (; pos.x < World::TileManager::getMapWidth(); pos.x += 16 * World::kTileSize)
            {
                auto tile = TileManager::get(pos);
                for (auto& el : tile)
                {
                    if (el.isGhost())
                    {
                        continue;
                    }

                    // If update removed/added tiles we must stop loop as pointer is invalid
                    if (!update(el, pos))
                    {
                        break;
                    }
                }
            }
            pos.x -= World::TileManager::getMapWidth();
        }
        pos.y -= World::TileManager::getMapHeight();

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
        const auto randFreq = gPrng2().randNext(20'003, 24'098);
        Audio::playSound(Audio::SoundId::demolishBuilding, pos, -1400, randFreq);
    }

    // 0x0042D8FF
    void removeBuildingElement(BuildingElement& elBuilding, const World::Pos2& pos)
    {
        if (!elBuilding.isGhost() && !elBuilding.isAiAllocated())
        {
            if (GameCommands::getUpdatingCompanyId() != CompanyId::neutral)
            {
                createDestructExplosion(World::Pos3(pos.x + 16, pos.y + 16, elBuilding.baseHeight()));
            }
        }

        if (elBuilding.sequenceIndex() == 0)
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

    static void setTerrainStyleAsCleared(const Pos2 pos, SurfaceElement& surface)
    {
        if (surface.isIndustrial())
        {
            return;
        }
        if (surface.getGrowthStage() > 0)
        {
            surface.setGrowthStage(0);
            surface.setSnowCoverage(0);

            Ui::ViewportManager::invalidate(pos, surface.baseHeight(), surface.baseHeight() + 32, ZoomLevel::eighth);
        }
        if (surface.snowCoverage() > 0)
        {
            surface.setSnowCoverage(0);

            Ui::ViewportManager::invalidate(pos, surface.baseHeight(), surface.baseHeight() + 32, ZoomLevel::eighth);
        }
    }

    // 0x004795D1
    void setLevelCrossingFlags(const World::Pos3 pos)
    {
        auto findLevelTrackAndRoad = [pos](auto&& trackFunction, auto&& roadFunction) {
            auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                if (el.baseHeight() != pos.z)
                {
                    continue;
                }
                if (el.isAiAllocated())
                {
                    continue;
                }
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack != nullptr)
                {
                    if (elTrack->trackId() == 0)
                    {
                        trackFunction(*elTrack);
                    }
                }
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad != nullptr)
                {
                    if (elRoad->roadId() == 0)
                    {
                        roadFunction(*elRoad);
                    }
                }
            }
        };

        bool hasRoad = false;
        bool hasTrack = false;
        findLevelTrackAndRoad(
            [&hasTrack](World::TrackElement& elTrack) { hasTrack |= elTrack.hasLevelCrossing(); },
            [&hasRoad](World::RoadElement& elRoad) { hasRoad |= elRoad.hasLevelCrossing(); });

        if (hasRoad ^ hasTrack)
        {
            findLevelTrackAndRoad(
                [hasTrack](World::TrackElement& elTrack) { if (hasTrack) { elTrack.setHasLevelCrossing(false); } },
                [hasRoad](World::RoadElement& elRoad) { if (hasRoad) {
                    elRoad.setHasLevelCrossing(false);
                    elRoad.setUnk7_10(false);
                    elRoad.setLevelCrossingObjectId(0);
                } });
        }
    }

    // 0x004690FC
    void setTerrainStyleAsCleared(const Pos2& pos)
    {
        auto* surface = World::TileManager::get(pos).surface();
        if (surface == nullptr)
        {
            return;
        }
        setTerrainStyleAsCleared(pos, *surface);
    }

    // 0x00469174
    void setTerrainStyleAsClearedAtHeight(const Pos3& pos)
    {
        auto* elSurface = World::TileManager::get(pos).surface();
        if (elSurface == nullptr)
        {
            return;
        }

        // If underground
        if (elSurface->baseHeight() > pos.z)
        {
            return;
        }
        // If quite high in the air
        if (pos.z - elSurface->baseHeight() >= 12 * World::kSmallZStep)
        {
            return;
        }

        setTerrainStyleAsCleared(pos, *elSurface);
    }

    // 0x00468651
    uint32_t adjustSurfaceHeight(World::Pos2 pos, SmallZ targetBaseZ, uint8_t slopeFlags, World::TileClearance::RemovedBuildings& removedBuildings, uint8_t flags)
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

            if (!SceneManager::isEditorMode())
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
                        auto args = FormatArguments::common();
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
                        auto args = FormatArguments::common();
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
        if (!SceneManager::isEditorMode())
        {
            // Reset terrain growth when not in editor
            surface->setGrowthStage(0);
        }

        surface->setBaseZ(targetBaseZ);
        surface->setClearZ(targetBaseZ);
        surface->setSlope(slopeFlags);

        landObj = ObjectManager::get<LandObject>(surface->terrain());
        if (landObj->hasFlags(LandObjectFlags::hasReplacementLandHeader) && !SceneManager::isEditorMode())
        {
            surface->setTerrain(landObj->replacementLandHeader);
        }

        if (surface->water() * kMicroToSmallZStep <= targetBaseZ)
        {
            surface->setWater(0);
        }

        mapInvalidateTileFull(pos);
        return totalCost;
    }

    // 0x004C4C28
    uint32_t adjustWaterHeight(World::Pos2 pos, SmallZ targetHeight, World::TileClearance::RemovedBuildings& removedBuildings, uint8_t flags)
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

            if (!SceneManager::isEditorMode())
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
            surface->setVariation(0);

            mapInvalidateTileFull(pos);
        }
        return totalCost;
    }

    // 0x0047AB9B
    void updateYearly()
    {
        const auto isObjectNotTram = getGameState().roadObjectIdIsNotTram;
        for (const auto& tilePos : getWorldRange())
        {
            auto tile = get(tilePos);
            for (auto& el : tile)
            {
                auto* elRoad = el.as<RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->isAiAllocated() || elRoad->isGhost())
                {
                    continue;
                }
                // This is a much cheaper tram checker
                // compared to getting the object
                if (isObjectNotTram & (1U << elRoad->roadObjectId()))
                {
                    elRoad->setUnk7_80(elRoad->hasUnk7_40());
                    elRoad->setUnk7_40(false);
                }
            }
        }
    }
}
