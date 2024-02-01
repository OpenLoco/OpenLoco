#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehiclePlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlace;

        VehiclePlacementArgs() = default;
        explicit VehiclePlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dx * World::kSmallZStep)
            , trackAndDirection(regs.bp)
            , trackProgress(regs.ebx >> 16)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx >> 16) == 0xFFFF)
        {
        }

        World::Pos3 pos;
        uint16_t trackAndDirection;
        uint16_t trackProgress;
        EntityId head;
        bool convertGhost = false;

        explicit operator registers() const
        {
            registers regs;
            regs.ebp = trackAndDirection;
            regs.di = enumValue(head);
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dx = pos.z / World::kSmallZStep;
            regs.ebx = convertGhost ? 0xFFFF0000 : (trackProgress << 16);
            return regs;
        }
    };
}
