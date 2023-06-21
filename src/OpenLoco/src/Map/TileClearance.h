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

    bool sub_462908(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool sub_462908(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc);
    bool sub_462917(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, uintptr_t clearFunctionLegacy);
    bool sub_462917(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt, std::function<ClearFuncResult(TileElement& el)> clearFunc);
    bool canConstructAt(const World::Pos2& pos, uint8_t baseZ, uint8_t clearZ, const QuarterTile& qt);

    struct LessThanPos3
    {
        bool operator()(World::Pos3 const& lhs, World::Pos3 const& rhs) const
        {
            return std::tie(lhs.x, lhs.y, lhs.z) < std::tie(rhs.x, rhs.y, rhs.z);
        }
    };

    ClearFuncResult tileClearFunction(World::TileElement& el, const World::Pos2 pos, std::set<World::Pos3, LessThanPos3>& removedBuildings, const uint8_t flags, currency32_t& cost);

    void registerHooks();
}
