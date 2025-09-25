#include "LowerRaiseLandMountain.h"
#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Terraform/LowerLand.h"
#include "GameCommands/Terraform/RaiseLand.h"
#include "Map/SurfaceData.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "ScenarioOptions.h"
#include "Types.hpp"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<uint32_t, 0x00F0014E> _mtnToolCost;
    static loco_global<uint8_t, 0x00F00154> _mtnToolGCFlags;
    static loco_global<uint8_t, 0x00F00155> _mtnToolHeightDiff;
    static loco_global<int8_t, 0x00F00156> _mtnToolOuterLoopIndex;

    static void adjustSurfaceSlope(Pos2 pos, int8_t targetBaseZ, uint8_t targetCorner, uint8_t referenceCornerFlag, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        if (!TileManager::validCoords(pos))
        {
            return;
        }

        auto tile = TileManager::get(pos);
        const auto* surface = tile.surface();
        SmallZ cornerBaseZ = TileManager::getSurfaceCornerDownHeight(*surface, referenceCornerFlag);
        int8_t baseZDiff = cornerBaseZ - targetBaseZ;
        uint8_t slopeFlags = 0;

        if (baseZDiff > 0)
        {
            if (baseZDiff <= _mtnToolHeightDiff)
            {
                return;
            }

            slopeFlags = lowerSurfaceCornerFlags(targetCorner, surface->slope());
            targetBaseZ = surface->baseZ();

            if (slopeFlags & SurfaceSlope::requiresHeightAdjustment)
            {
                targetBaseZ -= kSmallZStep;
                slopeFlags &= ~SurfaceSlope::requiresHeightAdjustment;
            }
        }
        else if (baseZDiff < 0)
        {
            if (-baseZDiff <= _mtnToolHeightDiff)
            {
                return;
            }

            slopeFlags = raiseSurfaceCornerFlags(targetCorner, surface->slope());
            targetBaseZ = surface->baseZ();

            if (slopeFlags & SurfaceSlope::requiresHeightAdjustment)
            {
                targetBaseZ += kSmallZStep;
                slopeFlags &= ~SurfaceSlope::requiresHeightAdjustment;
            }
        }
        else // if (baseZDiff == 0)
        {
            return;
        }

        auto result = TileManager::adjustSurfaceHeight(pos, targetBaseZ, slopeFlags, removedBuildings, _mtnToolGCFlags);
        if (result != FAILURE)
        {
            _mtnToolCost += result;
        }
    }

    // 0x004633F6
    static void adjustSurfaceSlopeSouth(Pos2 pos, int8_t targetBaseZ, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 2, SurfaceSlope::CornerUp::south, removedBuildings);
    }

    // 0x004634B9
    static void adjustSurfaceSlopeWest(Pos2 pos, int8_t targetBaseZ, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 3, SurfaceSlope::CornerUp::west, removedBuildings);
    }

    // 0x0046357C
    static void adjustSurfaceSlopeNorth(Pos2 pos, int8_t targetBaseZ, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 0, SurfaceSlope::CornerUp::north, removedBuildings);
    }

    // 0x0046363F
    static void adjustSurfaceSlopeEast(Pos2 pos, int8_t targetBaseZ, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 1, SurfaceSlope::CornerUp::east, removedBuildings);
    }

    static uint32_t adjustMountainCentre(const LowerRaiseLandMountainArgs& args, World::TileClearance::RemovedBuildings& removedBuildings, const uint8_t flags)
    {
        // Prepare parameters for raise/lower land tool
        uint32_t result = FAILURE;
        if (args.adjustment == 1)
        {
            RaiseLandArgs raiseArgs;
            raiseArgs.centre = args.centre;
            raiseArgs.pointA = args.pointA;
            raiseArgs.pointB = args.pointB;
            raiseArgs.corner = MapSelectionType::full;

            result = raiseLand(raiseArgs, removedBuildings, flags);
        }
        else
        {
            LowerLandArgs lowerArgs;
            lowerArgs.centre = args.centre;
            lowerArgs.pointA = args.pointA;
            lowerArgs.pointB = args.pointB;
            lowerArgs.corner = MapSelectionType::full;

            result = lowerLand(lowerArgs, removedBuildings, flags);
        }

        return result;
    }

    static void smoothenSurfaceNorth(const Pos2& refPos, const Pos2& targetPos, const LowerRaiseLandMountainArgs& args, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        auto tile = TileManager::get(refPos);
        auto surface = tile.surface();

        auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::south);
        adjustSurfaceSlopeEast(targetPos, height, removedBuildings);

        *_mtnToolHeightDiff -= kSmallZStep;

        if (targetPos.y >= args.pointA.y)
        {
            *_mtnToolHeightDiff += kSmallZStep;
            if (targetPos.y > args.pointB.y)
            {
                *_mtnToolHeightDiff += kSmallZStep;
            }
        }

        height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::west);
        adjustSurfaceSlopeNorth(targetPos, height, removedBuildings);
    }

    static void smoothenSurfaceEast(const Pos2& refPos, const Pos2& targetPos, const LowerRaiseLandMountainArgs& args, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        auto tile = TileManager::get(refPos);
        auto surface = tile.surface();

        auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::west);
        adjustSurfaceSlopeSouth(targetPos, height, removedBuildings);

        *_mtnToolHeightDiff -= kSmallZStep;

        if (targetPos.x >= args.pointA.x)
        {
            *_mtnToolHeightDiff += kSmallZStep;
            if (targetPos.x > args.pointB.x)
            {
                *_mtnToolHeightDiff += kSmallZStep;
            }
        }

        height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::north);
        adjustSurfaceSlopeEast(targetPos, height, removedBuildings);
    }

    static void smoothenSurfaceSouth(const Pos2& refPos, const Pos2& targetPos, const LowerRaiseLandMountainArgs& args, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        auto tile = TileManager::get(refPos);
        auto surface = tile.surface();

        auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::north);
        adjustSurfaceSlopeWest(targetPos, height, removedBuildings);

        *_mtnToolHeightDiff -= kSmallZStep;

        if (targetPos.y <= args.pointB.y)
        {
            *_mtnToolHeightDiff += kSmallZStep;
            if (targetPos.y < args.pointA.y)
            {
                *_mtnToolHeightDiff += kSmallZStep;
            }
        }

        height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::east);
        adjustSurfaceSlopeSouth(targetPos, height, removedBuildings);
    }

    static void smoothenSurfaceWest(const Pos2& refPos, const Pos2& targetPos, const LowerRaiseLandMountainArgs& args, World::TileClearance::RemovedBuildings& removedBuildings)
    {
        auto tile = TileManager::get(refPos);
        auto surface = tile.surface();

        auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::east);
        adjustSurfaceSlopeNorth(targetPos, height, removedBuildings);

        *_mtnToolHeightDiff -= kSmallZStep;

        if (targetPos.x <= args.pointB.x)
        {
            *_mtnToolHeightDiff += kSmallZStep;
            if (targetPos.x < args.pointA.x)
            {
                *_mtnToolHeightDiff += kSmallZStep;
            }
        }

        height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::south);
        adjustSurfaceSlopeWest(targetPos, height, removedBuildings);
    }

    // 0x00462DCE
    static uint32_t lowerRaiseLandMountain(const LowerRaiseLandMountainArgs& args, const uint8_t flags)
    {
        _mtnToolGCFlags = flags;

        if (flags & Flags::apply)
        {
            Scenario::getOptions().madeAnyChanges = 1;
        }

        _mtnToolCost = 0;

        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        World::TileClearance::RemovedBuildings removedBuildings{};

        // Play sound if this is a company action
        if ((flags & Flags::apply) && getCommandNestLevel() == 1 && getUpdatingCompanyId() != CompanyId::neutral)
        {
            const auto height = TileManager::getHeight(args.centre).landHeight;
            Audio::playSound(Audio::SoundId::construct, World::Pos3(args.centre.x, args.centre.y, height));
        }

        if (!TileManager::validCoords(args.pointA) || !TileManager::validCoords(args.pointB))
        {
            return FAILURE;
        }

        // First, raise/lower the mountain's centre tile
        {
            auto result = adjustMountainCentre(args, removedBuildings, flags);
            if (result != FAILURE)
            {
                _mtnToolCost = *_mtnToolCost + result;
            }
        }

        // 0x00462E7E
        auto preTile = TileManager::get(args.pointA);
        auto* preSurface = preTile.surface();
        if (preSurface->slope() != 0)
        {
            auto result = adjustMountainCentre(args, removedBuildings, flags);
            if (result != FAILURE)
            {
                _mtnToolCost = *_mtnToolCost + result;
            }

            // Verify that the slope is now gone. Abort if not.
            auto tile = TileManager::get(args.pointA);
            auto* surface = tile.surface();
            if (surface->slope() != 0)
            {
                // 0x004633CB
                GameCommands::setExpenditureType(ExpenditureType::Construction);

                auto tileHeight = World::TileManager::getHeight(args.centre);
                GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

                return _mtnToolCost;
            }
        }

        // 0x00462F25
        const tile_coord_t minRadius = ((args.pointB.x - args.pointA.x) / kTileSize) - 1;
        const tile_coord_t maxRadius = minRadius + 64;

        tile_coord_t radius = minRadius;
        auto basePos = args.pointA;
        _mtnToolOuterLoopIndex = -4;

        while (true)
        {
            *_mtnToolOuterLoopIndex += 4;
            radius += 2;
            basePos -= Pos2{ kTileSize, kTileSize };

            if (radius >= maxRadius)
            {
                break;
            }

            *_mtnToolHeightDiff = _mtnToolOuterLoopIndex * 2;

            {
                auto tile = TileManager::get(args.pointA);
                auto* surface = tile.surface();

                // 0x00462FAB
                auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::south);
                adjustSurfaceSlopeNorth(basePos, height, removedBuildings);
                basePos.y += kTileSize;
            }

            // 0x00462FCE
            for (auto i = radius; i > 0; i--)
            {
                auto pos = Pos2{ args.pointA.x, std::clamp(basePos.y, args.pointA.y, args.pointB.y) };
                smoothenSurfaceNorth(pos, basePos, args, removedBuildings);
                basePos.y += kTileSize;
            }

            // 0x00463089
            {
                auto tile = TileManager::get(args.pointA.x, args.pointB.y);
                auto* surface = tile.surface();

                // 0x004630C1
                auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::west);
                adjustSurfaceSlopeEast(basePos, height, removedBuildings);
                basePos.x += kTileSize;
            }

            // 0x004630E1
            for (auto i = radius; i > 0; i--)
            {
                auto pos = Pos2{ std::clamp(basePos.x, args.pointA.x, args.pointB.x), args.pointB.y };
                smoothenSurfaceEast(pos, basePos, args, removedBuildings);
                basePos.x += kTileSize;
            }

            // 0x00463199
            {
                auto tile = TileManager::get(args.pointB);
                auto* surface = tile.surface();

                // 0x004631D1
                auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::north);
                adjustSurfaceSlopeSouth(basePos, height, removedBuildings);
                basePos.y -= kTileSize;
            }

            // 0x004631F4
            for (auto i = radius; i > 0; i--)
            {
                auto pos = Pos2{ args.pointB.x, std::clamp(basePos.y, args.pointA.y, args.pointB.y) };
                smoothenSurfaceSouth(pos, basePos, args, removedBuildings);
                basePos.y -= kTileSize;
            }

            // 0x004632AF
            {
                auto tile = TileManager::get(Pos2(args.pointB.x, args.pointA.y));
                auto* surface = tile.surface();

                // 0x004632E7
                auto height = TileManager::getSurfaceCornerDownHeight(*surface, SurfaceSlope::CornerUp::east);
                adjustSurfaceSlopeWest(basePos, height, removedBuildings);
                basePos.x -= kTileSize;
            }

            // 0x0046330A
            for (auto i = radius; i > 0; i--)
            {
                auto pos = Pos2{ std::clamp(basePos.x, args.pointA.x, args.pointB.x), args.pointA.y };
                smoothenSurfaceWest(pos, basePos, args, removedBuildings);
                basePos.x -= kTileSize;
            }
        }

        // 0x004633CB
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.centre);
        GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

        return _mtnToolCost;
    }

    void lowerRaiseLandMountain(registers& regs)
    {
        const LowerRaiseLandMountainArgs args(regs);
        regs.ebx = lowerRaiseLandMountain(args, regs.bl);
    }
}
