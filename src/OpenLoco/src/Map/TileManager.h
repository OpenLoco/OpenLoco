#pragma once

#include "Tile.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>
#include <cstdint>
#include <functional>
#include <tuple>

namespace OpenLoco::World
{
    class QuarterTile;
    struct BuildingElement;
    enum class ElementType;
}

namespace OpenLoco::World::TileManager
{
    constexpr size_t maxElements = 0x6C000;

    enum class ClearResult
    {
        noCollision,
        collisionWithErrorMessage,
        collsionNoMessage
    };

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

    void initialise();
    stdx::span<TileElement> getElements();
    TileElement* getElementsEnd();
    uint32_t numFreeElements();
    TileElement** getElementIndex();
    Tile get(TilePos2 pos);
    Tile get(Pos2 pos);
    Tile get(coord_t x, coord_t y);
    void setElements(stdx::span<TileElement> elements);
    void removeElement(TileElement& element);
    TileElement* insertElement(ElementType type, const Pos2& pos, uint8_t baseZ, uint8_t occupiedQuads);
    template<typename TileT>
    TileT* insertElement(const Pos2& pos, const uint8_t baseZ, const uint8_t occupiedQuads)
    {
        return insertElement(TileT::kElementType, pos, baseZ, occupiedQuads)->template as<TileT>();
    }
    TileHeight getHeight(const Pos2& pos);
    void updateTilePointers();
    void reorganise();
    bool checkFreeElementsAndReorganise();
    bool sub_462908(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool sub_462908(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearResult(const TileElement& el)> clearFunc);
    bool sub_462917(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool sub_462917(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearResult(const TileElement& el)> clearFunc);
    bool canConstructAt(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt);
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
    void update();
    void updateYearly();
    void registerHooks();
    void removeSurfaceIndustry(const Pos2& pos);
    void createDestructExplosion(const World::Pos3& pos);
    void removeBuildingElement(BuildingElement& element, const World::Pos2& pos);
    void removeAllWallsOnTile(const World::TilePos2& pos, SmallZ baseZ);
}
