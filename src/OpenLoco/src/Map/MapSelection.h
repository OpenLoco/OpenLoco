#pragma once

#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Types.hpp>
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::World
{
    enum MapSelectionType : uint8_t
    {
        corner0,
        corner1,
        corner2,
        corner3,
        full,
        fullWater,
        quarter0,
        quarter1,
        quarter2,
        quarter3,
        edge0,
        edge1,
        edge2,
        edge3,
    };

    uint16_t setMapSelectionTiles(const Pos2& loc, const MapSelectionType selectionType);
    uint16_t setMapSelectionSingleTile(const Pos2& loc, bool setQuadrant = false);
    void mapInvalidateSelectionRect();
    void mapInvalidateMapSelectionTiles();
    void setMapSelectionArea(const Pos2& locA, const Pos2& locB);
    std::pair<Pos2, Pos2> getMapSelectionArea();
    void setMapSelectionCorner(const MapSelectionType corner);
    MapSelectionType getMapSelectionCorner();
    MapSelectionType getQuadrantOrCentreFromPos(const Pos2& loc);
    MapSelectionType getQuadrantFromPos(const Pos2& loc);
    MapSelectionType getSideFromPos(const Pos2& loc);
}
