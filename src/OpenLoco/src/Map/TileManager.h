#pragma once

#include "Tile.h"
#include "TileClearance.h"
#include <OpenLoco/Core/EnumFlags.hpp>
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

namespace OpenLoco::World::TileManager
{
    constexpr coord_t getMapRows() { return 384; }
    constexpr coord_t getMapColumns() { return 384; }
    constexpr coord_t kMapPitch = 512;
    constexpr uint32_t getMapHeight() { return getMapRows() * World::kTileSize; }
    constexpr uint32_t getMapWidth() { return getMapColumns() * World::kTileSize; }
    constexpr uint32_t getMapSize() { return getMapColumns() * getMapRows(); }

    constexpr size_t getMaxElements() { return 3U * getMapColumns() * getMapRows(); }
    constexpr size_t kMaxElementsOnOneTile = 1024; // If you exceed this then the game may buffer overflow in certain situations
    constexpr size_t getMaxUsableElements() { return getMaxElements() - kMaxElementsOnOneTile };
    const TileElement* const kInvalidTile = reinterpret_cast<const TileElement*>(static_cast<intptr_t>(-1));

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
    std::span<TileElement> getElements();
    TileElement* getElementsEnd();
    uint32_t numFreeElements();
    TileElement** getElementIndex();
    Tile get(TilePos2 pos);
    Tile get(Pos2 pos);
    Tile get(coord_t x, coord_t y);
    void setElements(std::span<TileElement> elements);
    void removeElement(TileElement& element);
    // This is used with wasRemoveOnLastElement to indicate that pointer passed to removeElement is now bad
    void setRemoveElementPointerChecker(TileElement& element);
    // See above. Used to indicate if pointer to removeElement is now bad
    bool wasRemoveOnLastElement();

    // Note: Any TileElement pointers invalid after this call
    TileElement* insertElement(ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    // Note: Any TileElement pointers invalid after this call
    template<typename TileT>
    TileT* insertElement(const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElement(TileT::kElementType, pos, baseZ, occupiedQuads)->template as<TileT>();
    }

    // Note: `after` pointer will be invalid after this call
    TileElement* insertElementAfterNoReorg(TileElement* after, ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    // Note: `after` pointer will be invalid after this call
    template<typename TileT>
    TileT* insertElementAfterNoReorg(TileElement* after, const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElementAfterNoReorg(after, TileT::kElementType, pos, baseZ, occupiedQuads)->template as<TileT>();
    }

    // Special road element insert
    World::RoadElement* insertElementRoad(const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);

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
    CompanyId getTileOwner(const World::TileElement& el);
    void mapInvalidateTileFull(World::Pos2 pos);
    void resetSurfaceClearance();
    int16_t mountainHeight(const World::Pos2& loc);
    uint16_t countSurroundingWaterTiles(const Pos2& pos);
    uint16_t countSurroundingDesertTiles(const Pos2& pos);
    uint16_t countSurroundingTrees(const Pos2& pos);
    uint16_t countNearbyWaterTiles(Pos2 pos);
    void update();
    void updateYearly();
    void registerHooks();
    void removeSurfaceIndustry(const Pos2& pos);
    void removeSurfaceIndustryAtHeight(const Pos3& pos);
    void createDestructExplosion(const World::Pos3& pos);
    void removeBuildingElement(BuildingElement& element, const World::Pos2& pos);
    void removeTree(TreeElement& element, const uint8_t flags, const World::Pos2& pos);
    void removeAllWallsOnTileAbove(const World::TilePos2& pos, SmallZ baseZ);
    void removeAllWallsOnTileBelow(const World::TilePos2& pos, SmallZ baseZ);
    void setLevelCrossingFlags(const World::Pos3 pos);
    void setTerrainStyleAsCleared(const Pos2& pos);
    void setTerrainStyleAsClearedAtHeight(const Pos3& pos);
    uint32_t adjustSurfaceHeight(World::Pos2 pos, SmallZ targetBaseZ, uint8_t slopeFlags, World::TileClearance::RemovedBuildings& removedBuildings, uint8_t flags);
    uint32_t adjustWaterHeight(World::Pos2 pos, SmallZ targetHeight, World::TileClearance::RemovedBuildings& removedBuildings, uint8_t flags);

    constexpr bool validCoord(const coord_t coord)
    {
        return (coord >= 0) && (coord < getMapWidth());
    }

    constexpr bool validTileCoord(const tile_coord_t coord)
    {
        return (coord >= 0) && (coord < getMapColumns());
    }

    constexpr bool validCoords(const Pos2& coords)
    {
        return validCoord(coords.x) && validCoord(coords.y);
    }

    constexpr bool validCoords(const TilePos2& coords)
    {
        return validTileCoord(coords.x) && validTileCoord(coords.y);
    }

    // drawing coordinates validation differs from general valid coordinate validation
    constexpr bool drawableCoord(const coord_t coord)
    {
        return (coord >= World::kTileSize) && (coord < (getMapWidth() - World::kTileSize - 1));
    }

    constexpr bool drawableTileCoord(const tile_coord_t coord)
    {
        return (coord >= 1) && (coord < (getMapColumns() - 2));
    }

    constexpr bool drawableCoords(const Pos2& coords)
    {
        return drawableCoord(coords.x) && drawableCoord(coords.y);
    }

    constexpr bool drawableCoords(const TilePos2& coords)
    {
        return drawableTileCoord(coords.x) && drawableTileCoord(coords.y);
    }

    constexpr coord_t clampCoord(coord_t coord)
    {
        return std::clamp<coord_t>(coord, 0, getMapWidth() - 1);
    }

    constexpr coord_t clampTileCoord(coord_t coord)
    {
        return std::clamp<coord_t>(coord, 0, getMapColumns() - 1);
    }
}
