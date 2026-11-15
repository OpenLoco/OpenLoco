#pragma once

#include "Tile.h"
#include "TileClearance.h"
#include "TileElement.h"
#include "TileElementEntry.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Store.hpp>
#include <array>
#include <cstdint>
#include <set>
#include <span>

namespace OpenLoco::World
{
    class QuarterTile;
    struct BuildingElement;
    struct TreeElement;
    struct SurfaceElement;
    struct RoadElement;
    enum class ElementType : uint8_t;
}

namespace OpenLoco::GameCommands
{
    enum class Flags : uint8_t;
}

namespace OpenLoco::World::TileManager
{
    const TileElement* const kInvalidTile = reinterpret_cast<const TileElement*>(static_cast<intptr_t>(-1));

    constexpr size_t kMaxElementsOnOneTile = 1024; // If you exceed this then the game may buffer overflow in certain situations

    const uint16_t kMinMapDimension = 16;
    const uint16_t kDefaultMapDimension = 384;
    // The largest map that can be created. World coordinates are 32 bit so the map extents are
    // limited by the tile pointer array and the entity spatial index (which are both sized from
    // the map pitch) rather than by the coordinate type.
    const uint16_t kMaxMapDimension = 4096;

    enum class ElementPositionFlags : uint8_t
    {
        none = 0U,
        aboveGround = 1U << 0,
        underground = 1U << 1,
        partiallyUnderwater = 1U << 2,
        underwater = 1U << 3,

        anyHeightBuildingCollisions = 1U << 7, // Do not use only for interop
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ElementPositionFlags);

    void allocateMapElements();
    void initialise();
    std::span<const TileElementEntry> getEntries();
    uint32_t numFreeElements();
    Tile get(TilePos2 pos);
    Tile get(Pos2 pos);
    Tile get(coord_t x, coord_t y);
    void removeElement(TileElementEntry& entry);
    // This is used with wasRemoveOnLastElement to indicate that pointer passed to removeElement is now bad
    void setRemoveElementPointerChecker(TileElementEntry& entry);
    // See above. Used to indicate if pointer to removeElement is now bad
    bool wasRemoveOnLastElement();

    // Note: Any TileElementEntry pointers invalid after this call
    TileElementEntry* insertElement(ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    // Note: Any TileElementEntry pointers invalid after this call
    template<typename TileT>
    TileElementEntry* insertElement(const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElement(TileT::kElementType, pos, baseZ, occupiedQuads);
    }

    // Note: `after` pointer will be invalid after this call
    TileElementEntry* insertElementAfterNoReorg(TileElementEntry* after, ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    // Note: `after` pointer will be invalid after this call
    template<typename TileT>
    TileElementEntry* insertElementAfterNoReorg(TileElementEntry* after, const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElementAfterNoReorg(after, TileT::kElementType, pos, baseZ, occupiedQuads);
    }

    // Special road element insert
    TileElementEntry* insertElementRoad(const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);

    TileHeight getHeight(const Pos2& pos);
    SmallZ getSurfaceCornerHeight(const SurfaceElement& surface);
    SmallZ getSurfaceCornerDownHeight(const SurfaceElement& surface, const uint8_t cornerMask);
    void updateTilePointers();
    // Only disables first call to defrag
    void disablePeriodicDefrag();
    // Fully defragment the tile element array
    void reorganise();
    // Defragments singular tile (chosen tile updates each call)
    void defragmentTilePeriodic();
    bool checkFreeElementsAndReorganise();
    TileElement& resolveEntry(const TileElementEntry* entry);

    template<typename T>
    Store<T>& getStore();

    template<typename T>
    TileElementEntry allocElement(const T& element)
    {
        Store<T>& store = getStore<T>();
        const uint32_t idx = store.allocate();
        store[idx] = element;

        TileElementEntry entry{};
        entry.setType(T::kElementType);
        entry.setIndex(idx);
        return entry;
    }

    void destroyElement(const TileElementEntry& entry);

    CompanyId getTileOwner(const World::TileElementEntry& el);
    void mapInvalidateTileFull(World::Pos2 pos);
    void resetSurfaceClearance();
    int16_t mountainHeight(const World::Pos2& loc);
    uint16_t countSurroundingWaterTiles(const Pos2& pos);
    uint16_t countSurroundingDesertTiles(const Pos2& pos);
    uint16_t countSurroundingTrees(const Pos2& pos);
    uint16_t countNearbyWaterTiles(Pos2 pos);
    void tick();
    void updateYearly();
    void removeSurfaceIndustry(const Pos2& pos);
    void removeSurfaceIndustryAtHeight(const Pos3& pos);
    void createDestructExplosion(const World::Pos3& pos);
    void removeBuildingElement(TileElementEntry& entry, const World::Pos2& pos);
    void removeTree(TileElementEntry& entry, const GameCommands::Flags flags, const World::Pos2& pos);
    void removeAllWallsOnTileAbove(const World::TilePos2& pos, SmallZ baseZ);
    void removeAllWallsOnTileBelow(const World::TilePos2& pos, SmallZ baseZ);
    void setLevelCrossingFlags(const World::Pos3 pos);
    void setTerrainStyleAsCleared(const Pos2& pos);
    void setTerrainStyleAsClearedAtHeight(const Pos3& pos);
    uint32_t adjustSurfaceHeight(World::Pos2 pos, SmallZ targetBaseZ, uint8_t slopeFlags, World::TileClearance::RemovedBuildings& removedBuildings, GameCommands::Flags flags);
    uint32_t adjustWaterHeight(World::Pos2 pos, SmallZ targetHeight, World::TileClearance::RemovedBuildings& removedBuildings, GameCommands::Flags flags);

    void setMapSize(coord_t cols, coord_t rows);
    coord_t getMapRows();
    coord_t getMapColumns();
    // Number of slots per row of the tile pointer array. Always >= max(columns, rows) so that
    // tiles of different rows never share a slot (see getTileIndex).
    coord_t getMapPitch();
    int32_t getMapHeight();
    int32_t getMapWidth();
    uint32_t getMapSize();
    uint32_t getMaxElements();

    bool validCoords(const Pos2& coords);
    bool validCoords(const TilePos2& coords);
    TilePos2 clampTileCoords(const TilePos2& coords);
    Pos2 clampCoords(const Pos2& coords);

    // drawing coordinates validation differs from general valid coordinate validation
    bool drawableCoords(const Pos2& coords);
    bool drawableTileCoords(const TilePos2& coords);
}
