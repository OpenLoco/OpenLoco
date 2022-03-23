#pragma once

#include "../Core/Span.hpp"
#include "Tile.h"
#include <cstdint>
#include <tuple>

namespace OpenLoco::Map::TileManager
{
    constexpr size_t maxElements = 0x6C000;

    void initialise();
    stdx::span<TileElement> getElements();
    TileElement* getElementsEnd();
    TileElement** getElementIndex();
    Tile get(const TilePos2& pos);
    Tile get(const Pos2& pos);
    Tile get(coord_t x, coord_t y);
    void setElements(stdx::span<TileElement> elements);
    void removeElement(TileElement& element);
    TileHeight getHeight(const Pos2& pos);
    void updateTilePointers();
    void reorganise();
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
    uint16_t countSurroundingWaterTiles(const Pos2& pos);
    uint16_t countSurroundingTrees(const Pos2& pos);
    void update();
    void updateYearly();
    void registerHooks();
    void removeSurfaceIndustry(const Pos2& pos);
    void createDestructExplosion(const Map::Pos3& pos);
    void removeBuildingElement(BuildingElement& element, const Map::Pos2& pos);

    template<typename T, typename Visitor>
    void visitAll(const TilePos2& pos, Visitor&& vis)
    {
        auto tile = get(pos);
        for (auto& el : tile)
        {
            auto* elT = el.as<T>();
            if (elT == nullptr)
            {
                continue;
            }
            vis(*elT);
        }
    }
    template<typename T, typename Visitor>
    T* visitFirst(const TilePos2& pos, Visitor&& vis)
    {
        auto tile = get(pos);
        for (auto& el : tile)
        {
            auto* elT = el.as<T>();
            if (elT == nullptr)
            {
                continue;
            }
            vis(*elT);
            return elT;
        }
    }
}
