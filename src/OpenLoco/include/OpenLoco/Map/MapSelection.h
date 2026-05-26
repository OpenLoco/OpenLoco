#pragma once

#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Types.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <span>

namespace OpenLoco::World
{
    enum class MapSelectionFlags : uint8_t
    {
        none = 0U,
        enable = 1U << 0,
        enableConstruct = 1U << 1,
        enableConstructionArrow = 1U << 2,
        unk_03 = 1U << 3,
        unk_04 = 1U << 4, // Vehicle orders?
        catchmentArea = 1U << 5,
        hoveringOverStation = 1U << 6,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(MapSelectionFlags);

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

    struct ConstructionArrow
    {
        World::Pos3 pos;
        uint8_t direction;
    };

    uint16_t setMapSelectionTiles(const Pos2& loc, const MapSelectionType selectionType, uint16_t toolSize);
    uint16_t setMapSelectionSingleTile(const Pos2& loc, bool setQuadrant = false);
    void mapInvalidateSelectionRect();

    void resetMapSelectionFreeFormTiles();
    void addMapSelectionFreeFormTile(const Pos2& pos);
    std::span<const Pos2> getMapSelectionFreeFormTiles();
    void mapInvalidateMapSelectionFreeFormTiles();
    bool isWithinMapSelectionFreeFormTiles(const Pos2 pos);

    void setMapSelectionArea(const Pos2& locA, const Pos2& locB);
    std::pair<Pos2, Pos2> getMapSelectionArea();
    void setMapSelectionCorner(const MapSelectionType corner);

    MapSelectionType getMapSelectionCorner();
    MapSelectionType getQuadrantOrCentreFromPos(const Pos2& loc);
    MapSelectionType getQuadrantFromPos(const Pos2& loc);
    MapSelectionType getSideFromPos(const Pos2& loc);

    MapSelectionFlags getMapSelectionFlags();
    bool hasMapSelectionFlag(MapSelectionFlags flags);
    void setMapSelectionFlags(MapSelectionFlags flags);
    void resetMapSelectionFlag(MapSelectionFlags flags);
    void resetMapSelectionFlags();

    const ConstructionArrow& getConstructionArrow();
    void setConstructionArrow(const ConstructionArrow& arrow);
}
