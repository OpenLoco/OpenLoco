#pragma once

#include "Tile.h"
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
    enum class ElementType : uint8_t;
}

namespace OpenLoco::World::TileManager
{
    constexpr size_t maxElements = 0x6C000;
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
    TileElement* insertElement(ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    template<typename TileT>
    TileT* insertElement(const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElement(TileT::kElementType, pos, baseZ, occupiedQuads)->template as<TileT>();
    }

    TileElement* insertElementAfterNoReorg(TileElement* after, ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    template<typename TileT>
    TileT* insertElementAfterNoReorg(TileElement* after, const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElementAfterNoReorg(after, TileT::kElementType, pos, baseZ, occupiedQuads)->template as<TileT>();
    }
    TileHeight getHeight(const Pos2& pos);
    SmallZ getSurfaceCornerHeight(const SurfaceElement& surface);
    void updateTilePointers();
    void reorganise();
    bool checkFreeElementsAndReorganise();
    CompanyId getTileOwner(const World::TileElement& el);
    uint16_t setMapSelectionTiles(const World::Pos2& loc, const uint8_t selectionType);
    uint16_t setMapSelectionSingleTile(const World::Pos2& loc, bool setQuadrant = false);
    void mapInvalidateSelectionRect();
    void mapInvalidateTileFull(World::Pos2 pos);
    void mapInvalidateMapSelectionTiles();
    void setMapSelectionArea(const Pos2& locA, const Pos2& locB);
    std::pair<Pos2, Pos2> getMapSelectionArea();
    void setMapSelectionCorner(const uint8_t corner);
    uint8_t getMapSelectionCorner();
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
    void createDestructExplosion(const World::Pos3& pos);
    void removeBuildingElement(BuildingElement& element, const World::Pos2& pos);
    void removeTree(TreeElement& element, const uint8_t flags, const World::Pos2& pos);
    void removeAllWallsOnTileAbove(const World::TilePos2& pos, SmallZ baseZ);
    void removeAllWallsOnTileBelow(const World::TilePos2& pos, SmallZ baseZ);
    void setTerrainStyleAsCleared(const Pos2& pos);
    uint32_t adjustSurfaceHeight(World::Pos2 pos, SmallZ targetBaseZ, uint8_t slopeFlags, std::set<World::Pos3, LessThanPos3>& removedBuildings, uint8_t flags);
    uint32_t adjustWaterHeight(World::Pos2 pos, SmallZ targetHeight, std::set<World::Pos3, LessThanPos3>& removedBuildings, uint8_t flags);
}
