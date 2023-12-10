#include "LowerRaiseLandMountain.h"
#include "GameCommands/GameCommands.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Types.hpp"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<uint32_t, 0x00F0014E> mtnToolCost;
    static loco_global<uint8_t, 0x00F00154> mtnToolGCFlags;
    static loco_global<uint8_t, 0x00F00155> _F00155;

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

    // 0x004633F6
    void sub_4633F6(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        if (!validCoords(pos))
        {
            return;
        }

        // Logging::info("sub_4633F6 with x={}, y={}", pos.x, pos.y);

        auto tile = TileManager::get(pos);
        const auto* surface = tile.surface();
        auto cornerBaseZ = TileManager::getSurfaceCornerHeight(*surface, SurfaceSlope::CornerUp::south);
        auto baseZDiff = cornerBaseZ - targetBaseZ;
        uint8_t slopeFlags;

        // 0x00463450
        if (baseZDiff == 0)
        {
            return;
        }
        else if (baseZDiff > 0)
        {
            if (baseZDiff <= _F00155)
            {
                return;
            }

            // TODO: only user; integrate
            static loco_global<uint8_t[32], 0x004FD41C> _4FD41C;
            slopeFlags = _4FD41C[surface->slope()];

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

            // TODO: only user; integrate
            static loco_global<uint8_t[32], 0x004FD37C> _4FD37C;
            slopeFlags = _4FD37C[surface->slope()];

            targetBaseZ = surface->baseZ();
            if (slopeFlags & SurfaceSlope::requiresHeightAdjustment)
            {
                targetBaseZ += kSmallZStep;
                slopeFlags &= ~SurfaceSlope::requiresHeightAdjustment;
            }
        }

        auto result = TileManager::adjustSurfaceHeight(pos, targetBaseZ, slopeFlags, removedBuildings, mtnToolGCFlags);
        if (result != FAILURE)
        {
            mtnToolCost += result;
        }
    }

    // 0x004634B9
    void sub_4634B9(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        if (!validCoords(pos))
        {
            return;
        }

        // Logging::info("sub_4634B9 with x={}, y={}", pos.x, pos.y);

        auto tile = TileManager::get(pos);
        const auto* surface = tile.surface();
        auto cornerBaseZ = TileManager::getSurfaceCornerHeight(*surface, SurfaceSlope::CornerUp::west);
        auto baseZDiff = cornerBaseZ - targetBaseZ;
        uint8_t slopeFlags;

        // 0x00463513
        if (baseZDiff == 0)
        {
            return;
        }
        else if (baseZDiff > 0)
        {
            if (baseZDiff <= _F00155)
            {
                return;
            }

            // TODO: only user; integrate
            static loco_global<uint8_t[32], 0x004FD43C> _4FD43C;
            slopeFlags = _4FD43C[surface->slope()];

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

            // TODO: only user; integrate
            static loco_global<uint8_t[32], 0x004FD39C> _4FD39C;
            slopeFlags = _4FD39C[surface->slope()];

            targetBaseZ = surface->baseZ();
            if (slopeFlags & SurfaceSlope::requiresHeightAdjustment)
            {
                targetBaseZ += kSmallZStep;
                slopeFlags &= ~SurfaceSlope::requiresHeightAdjustment;
            }
        }

        auto result = TileManager::adjustSurfaceHeight(pos, targetBaseZ, slopeFlags, removedBuildings, mtnToolGCFlags);
        if (result != FAILURE)
        {
            mtnToolCost += result;
        }
    }

    // 0x0046357C
    void sub_46357C(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        if (!validCoords(pos))
        {
            return;
        }

        // Logging::info("sub_46357C with x={}, y={}", pos.x, pos.y);

        auto tile = TileManager::get(pos);
        const auto* surface = tile.surface();
        auto baseZ = TileManager::getSurfaceCornerHeight(*surface);

        // 0x004635D6
        if (baseZ < targetBaseZ)
        {
            return;
        }
        else
        {
        }
    }

    // 0x0046363F
    void sub_46363F(Pos2 pos, int8_t targetBaseZ, std::set<Pos3, LessThanPos3>& removedBuildings)
    {
        if (!validCoords(pos))
        {
            return;
        }

        // Logging::info("sub_46363F with x={}, y={}", pos.x, pos.y);

        auto tile = TileManager::get(pos);
        const auto* surface = tile.surface();
        auto baseZ = TileManager::getSurfaceCornerHeight(*surface);

        // 0x00463699
        if (baseZ < targetBaseZ)
        {
            return;
        }
        else
        {
        }
    }
}
