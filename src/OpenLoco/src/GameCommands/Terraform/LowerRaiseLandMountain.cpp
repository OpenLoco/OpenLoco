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
    // TODO: remove after implementing the actual game command
    void registerMountainHooks()
    {
        registerHook(
            0x004633F6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4633F6({ regs.ax, regs.cx }, regs.dl);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004634B9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4634B9({ regs.ax, regs.cx }, regs.dl);
                regs = backup;
                return 0;
            });

        registerHook(
            0x0046357C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_46357C({ regs.ax, regs.cx }, regs.dl);
                regs = backup;
                return 0;
            });

        registerHook(
            0x0046363F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_46363F({ regs.ax, regs.cx }, regs.dl);
                regs = backup;
                return 0;
            });
    }

    // 0x004633F6
    void sub_4633F6(Pos2 pos, int8_t dl)
    {
        if (!validCoords(pos))
        {
            return;
        }

        auto tile = TileManager::get(*pos);
        const auto* surface = tile.surface();
        auto baseZ = TileManager::getSurfaceCornerHeight(*surface);

        // 0x00463513
        if (baseZ < dl)
        {
            continue;
        }
        else
        {
        }

        // Logging::info("sub_4633F6 with x={}, y={}", pos.x, pos.y);
    }

    // 0x004634B9
    void sub_4634B9(Pos2 pos, int8_t dl)
    {
        if (!validCoords(pos))
        {
            return;
        }

        auto tile = TileManager::get(*pos);
        const auto* surface = tile.surface();
        auto baseZ = TileManager::getSurfaceCornerHeight(*surface);

        // 0x00463513
        if (baseZ < dl)
        {
            continue;
        }
        else
        {
        }
        // Logging::info("sub_4634B9 with x={}, y={}", pos.x, pos.y);
    }

    // 0x0046357C
    void sub_46357C(Pos2 pos, int8_t dl)
    {
        if (!validCoords(pos))
        {
            return;
        }

        auto tile = TileManager::get(*pos);
        const auto* surface = tile.surface();
        auto baseZ = TileManager::getSurfaceCornerHeight(*surface);

        // 0x004635D6
        if (baseZ < dl)
        {
            continue;
        }
        else
        {
        }
        // Logging::info("sub_46357C with x={}, y={}", pos.x, pos.y);
    }

    // 0x0046363F
    void sub_46363F(Pos2 pos, int8_t dl)
    {
        if (!validCoords(pos))
        {
            return;
        }

        auto tile = TileManager::get(*pos);
        const auto* surface = tile.surface();
        auto baseZ = TileManager::getSurfaceCornerHeight(*surface);

        // 0x00463699
        if (baseZ < dl)
        {
            continue;
        }
        else
        {
        }
        // Logging::info("sub_46363F with x={}, y={}", pos.x, pos.y);
    }
}
