#include "LowerRaiseLandMountain.h"
#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Terraform/LowerLand.h"
#include "GameCommands/Terraform/RaiseLand.h"
#include "Map/SurfaceData.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "S5/S5.h"
#include "Types.hpp"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<uint32_t, 0x00F00146> mtnToolXParams;
    static loco_global<uint32_t, 0x00F0014A> mtnToolYParams;
    static loco_global<uint32_t, 0x00F0014E> mtnToolCost;
    static loco_global<uint8_t, 0x00F00154> mtnToolGCFlags;
    static loco_global<uint8_t, 0x00F00155> _F00155;
    static loco_global<uint8_t, 0x00F00156> _F00156;

    static void adjustSurfaceSlope(Pos2 pos, int8_t targetBaseZ, uint8_t targetCorner, uint8_t referenceCornerFlag, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        if (!validCoords(pos))
        {
            return;
        }

        auto tile = TileManager::get(pos);
        const auto* surface = tile.surface();
        SmallZ cornerBaseZ = TileManager::getSurfaceCornerHeight(*surface, referenceCornerFlag);
        int8_t baseZDiff = cornerBaseZ - targetBaseZ;
        uint8_t slopeFlags = 0;

        if (baseZDiff > 0)
        {
            if (baseZDiff <= _F00155)
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
            if (-baseZDiff <= _F00155)
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

        auto result = TileManager::adjustSurfaceHeight(pos, targetBaseZ, slopeFlags, removedBuildings, mtnToolGCFlags);
        if (result != FAILURE)
        {
            mtnToolCost += result;
        }
    }

    // 0x004633F6
    static void sub_4633F6(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 2, SurfaceSlope::CornerUp::south, removedBuildings);
    }

    // 0x004634B9
    static void sub_4634B9(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 3, SurfaceSlope::CornerUp::west, removedBuildings);
    }

    // 0x0046357C
    static void sub_46357C(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 0, SurfaceSlope::CornerUp::north, removedBuildings);
    }

    // 0x0046363F
    static void sub_46363F(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        adjustSurfaceSlope(pos, targetBaseZ, 1, SurfaceSlope::CornerUp::east, removedBuildings);
    }

    // TODO: remove after implementing the actual game command
    void registerMountainHooks()
    {
        registerHook(
            0x004633F6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                // TODO: hacky workaround
                std::set<World::Pos3, LessThanPos3> removedBuildings{};

                registers backup = regs;
                sub_4633F6({ regs.ax, regs.cx }, regs.dl, removedBuildings);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004634B9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                // TODO: hacky workaround
                std::set<World::Pos3, LessThanPos3> removedBuildings{};

                registers backup = regs;
                sub_4634B9({ regs.ax, regs.cx }, regs.dl, removedBuildings);
                regs = backup;
                return 0;
            });

        registerHook(
            0x0046357C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                // TODO: hacky workaround
                std::set<World::Pos3, LessThanPos3> removedBuildings{};

                registers backup = regs;
                sub_46357C({ regs.ax, regs.cx }, regs.dl, removedBuildings);
                regs = backup;
                return 0;
            });

        registerHook(
            0x0046363F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                // TODO: hacky workaround
                std::set<World::Pos3, LessThanPos3> removedBuildings{};

                registers backup = regs;
                sub_46363F({ regs.ax, regs.cx }, regs.dl, removedBuildings);
                regs = backup;
                return 0;
            });
    }

    // 0x00462DCE
    static uint32_t lowerRaiseLandMountain(const LowerRaiseLandMountainArgs& args, const uint8_t flags)
    {
        mtnToolXParams = (args.pointB.x << 16) | args.pointA.x;
        mtnToolYParams = (args.pointB.y << 16) | args.pointA.y;
        mtnToolGCFlags = flags;

        if (flags & Flags::apply)
        {
            S5::getOptions().madeAnyChanges = 1;
        }

        mtnToolCost = 0;

        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        std::set<World::Pos3, LessThanPos3> removedBuildings{};

        // Play sound if this is a company action
        if ((flags & Flags::apply) && getCommandNestLevel() == 1 && getUpdatingCompanyId() != CompanyId::neutral)
        {
            const auto height = TileManager::getHeight(args.centre).landHeight;
            Audio::playSound(Audio::SoundId::construct, World::Pos3(args.centre.x, args.centre.y, height));
        }

        // First, raise/lower the mountain's centre tile
        {
            // Prepare parameters for raise/lower land tool
            uint32_t result = FAILURE;
            if (args.adjustment == 1)
            {
                RaiseLandArgs raiseArgs;
                raiseArgs.centre = args.centre;
                raiseArgs.pointA = args.pointA;
                raiseArgs.pointB = args.pointB;
                raiseArgs.corner = 4;

                result = raiseLand(raiseArgs, removedBuildings, flags);
            }
            else
            {
                LowerLandArgs lowerArgs;
                lowerArgs.centre = args.centre;
                lowerArgs.pointA = args.pointA;
                lowerArgs.pointB = args.pointB;
                lowerArgs.corner = 4;

                result = lowerLand(lowerArgs, removedBuildings, flags);
            }

            if (result != FAILURE)
            {
                mtnToolCost = *mtnToolCost + result;
            }
        }

        // 0x00462E7E
        auto tile = TileManager::get(args.pointA.x, args.pointA.y);
        auto* surface = tile.surface();
        if (surface->slope() != 0)
        {
            // Prepare parameters for raise/lower land tool
            uint32_t result = FAILURE;
            if (args.adjustment == -1)
            {
                RaiseLandArgs raiseArgs;
                raiseArgs.centre = args.centre;
                raiseArgs.pointA = args.pointA;
                raiseArgs.pointB = args.pointB;
                raiseArgs.corner = 4;

                result = raiseLand(raiseArgs, removedBuildings, flags);
            }
            else
            {
                LowerLandArgs lowerArgs;
                lowerArgs.centre = args.centre;
                lowerArgs.pointA = args.pointA;
                lowerArgs.pointB = args.pointB;
                lowerArgs.corner = 4;

                result = lowerLand(lowerArgs, removedBuildings, flags);
            }

            // TODO: from game command result
            if (result != FAILURE)
            {
                mtnToolCost = *mtnToolCost + result;
            }

            // Verify that the slope is now gone. Abort if not.
            auto tile = TileManager::get(args.pointA.x, args.pointA.y);
            auto* surface = tile.surface();
            if (surface->slope() != 0)
            {
                // 0x004633CB
                GameCommands::setExpenditureType(ExpenditureType::Construction);

                auto tileHeight = World::TileManager::getHeight(args.centre);
                GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

                return mtnToolCost;
            }
        }

        // 0x004633CB
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.centre);
        GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

        return mtnToolCost;
    }

    void lowerRaiseLandMountain(registers& regs)
    {
        const LowerRaiseLandMountainArgs args(regs);
        regs.ebx = lowerRaiseLandMountain(args, regs.bl);
    }
}
