#pragma once

#include "Economy/Currency.h"
#include "QuarterTile.h"
#include "Tile.h"
#include <functional>
#include <sfl/small_set.hpp>

namespace OpenLoco::World
{
    struct BuildingElement;
    struct TreeElement;
}

namespace OpenLoco::World::TileManager
{
    enum class ElementPositionFlags : uint8_t;
}

namespace OpenLoco::World::TileClearance
{
    enum class ClearFuncResult
    {
        allCollisionsRemoved,
        collision,
        collisionErrorSet,
        noCollision,
        collisionRemoved,
    };

    void setCollisionErrorMessage(const World::TileElement& el);

    bool applyClearAtAllHeights(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool applyClearAtAllHeights(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc);
    bool applyClearAtStandardHeight(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool applyClearAtStandardHeight(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc);
    bool canConstructAt(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt);

    using RemovedBuildings = sfl::small_set<World::Pos3, 128, LessThanPos3>;

    // Removes Buildings and Trees but everything else is a collision
    ClearFuncResult clearWithDefaultCollision(World::TileElement& el, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost);
    // Removes Buildings and Trees but everything else is **NOT** a collision
    ClearFuncResult clearWithoutDefaultCollision(World::TileElement& el, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost);
    // Removes a building as per normal clear function setup
    ClearFuncResult clearBuildingCollision(World::BuildingElement& elBuilding, const World::Pos2 pos, RemovedBuildings& removedBuildings, const uint8_t flags, currency32_t& cost);
    // Removes a tree as per normal clear function setup
    ClearFuncResult clearTreeCollision(World::TreeElement& elTree, const World::Pos2 pos, const uint8_t flags, currency32_t& cost);

    // These are an additional return variable from the applyClear functions
    TileManager::ElementPositionFlags getPositionFlags();

    void registerHooks();
}
