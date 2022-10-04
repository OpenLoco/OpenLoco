#pragma once

#include "../Core/Span.hpp"
#include "Tile.h"
#include <cstdint>
#include <tuple>

namespace OpenLoco::Map
{
    class QuarterTile;
}

namespace OpenLoco::Map::TileManager
{
    constexpr size_t maxElements = 0x6C000;

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
    bool canConstructAt(const Map::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt);
    uint16_t setMapSelectionTiles(const Map::Pos2& loc, const uint8_t selectionType);
    uint16_t setMapSelectionSingleTile(const Map::Pos2& loc, bool setQuadrant = false);
    void mapInvalidateSelectionRect();
    void mapInvalidateTileFull(Map::Pos2 pos);
    void mapInvalidateMapSelectionTiles();
    void setMapSelectionArea(const Pos2& locA, const Pos2& locB);
    std::pair<Pos2, Pos2> getMapSelectionArea();
    void setMapSelectionCorner(const uint8_t corner);
    uint8_t getMapSelectionCorner();
    void resetSurfaceClearance();
    int16_t mountainHeight(const Map::Pos2& loc);
    uint16_t countSurroundingWaterTiles(const Pos2& pos);
    uint16_t countSurroundingDesertTiles(const Pos2& pos);
    uint16_t countSurroundingTrees(const Pos2& pos);
    void update();
    void updateYearly();
    void registerHooks();
    void removeSurfaceIndustry(const Pos2& pos);
    void createDestructExplosion(const Map::Pos3& pos);
    void removeBuildingElement(BuildingElement& element, const Map::Pos2& pos);
}
