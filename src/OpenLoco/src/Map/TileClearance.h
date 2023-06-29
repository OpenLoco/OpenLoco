#pragma once

#include "Economy/Currency.h"
#include "QuarterTile.h"
#include "Tile.h"
#include <functional>
#include <set>

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

    bool applyClearAtAllHeights(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool applyClearAtAllHeights(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc);
    bool applyClearAtStandardHeight(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool applyClearAtStandardHeight(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc);
    bool canConstructAt(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt);

    ClearFuncResult clearWithDefaultCollision(World::TileElement& el, const World::Pos2 pos, std::set<World::Pos3, LessThanPos3>& removedBuildings, const uint8_t flags, currency32_t& cost);
    ClearFuncResult clearWithoutDefaultCollision(World::TileElement& el, const World::Pos2 pos, std::set<World::Pos3, LessThanPos3>& removedBuildings, const uint8_t flags, currency32_t& cost);

    void registerHooks();
}
