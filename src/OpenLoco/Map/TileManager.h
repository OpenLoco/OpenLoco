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
    Tile get(TilePos2 pos);
    Tile get(Pos2 pos);
    Tile get(coord_t x, coord_t y);
    void setElements(stdx::span<TileElement> elements);
    TileHeight getHeight(const Pos2& pos);
    void updateTilePointers();
    void reorganise();
    Pos2 screenGetMapXY(int16_t x, int16_t y);
    uint16_t setMapSelectionTiles(int16_t x, int16_t y);
    Pos3 screenPosToMapPos(int16_t x, int16_t y);
    uint16_t setMapSelectionSingleTile(int16_t x, int16_t y, bool setQuadrant = false);
    void mapInvalidateSelectionRect();
    void mapInvalidateTileFull(Map::Pos2 pos);
    void mapInvalidateMapSelectionTiles();
    void setMapSelectionArea(const Pos2& locA, const Pos2& locB);
    std::pair<Pos2, Pos2> getMapSelectionArea();
    void setMapSelectionCorner(const uint8_t corner);
    uint8_t getMapSelectionCorner();
    void resetSurfaceClearance();
    void createAnimation(uint8_t type, const Pos2& pos, tile_coord_t baseZ);
    void resetAnimations();
    uint16_t countSurroundingWaterTiles(const Pos2& pos);
    uint16_t countSurroundingTrees(const Pos2& pos);
    void registerHooks();
}
