#pragma once

#include "../Core/Span.hpp"
#include "Tile.h"
#include <cstdint>
#include <tuple>

namespace OpenLoco::Map::TileManager
{
    namespace MapSelectFlag
    {
        constexpr uint16_t enable = (1 << 0);
        constexpr uint16_t enableConstruct = (1 << 1);
    }

    constexpr size_t maxElements = 0x6C000;

    void initialise();
    stdx::span<TileElement> getElements();
    TileElement* getElementsEnd();
    TileElement** getElementIndex();
    Tile get(TilePos2 pos);
    Tile get(Pos2 pos);
    Tile get(coord_t x, coord_t y);
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
}
