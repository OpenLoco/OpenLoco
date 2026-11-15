#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleWaterPlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlaceWater;

        VehicleWaterPlacementArgs() = default;
        explicit VehicleWaterPlacementArgs(const registers& regs)
            : pos(regs.eax, regs.ecx, regs.dx)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx & 0xFFFF0000) == 0xFFFF0000)
        {
        }

        World::Pos3 pos;
        EntityId head;
        bool convertGhost = false;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.dx = pos.z;
            regs.di = enumValue(head);
            regs.ebx = convertGhost ? 0xFFFF0000 : 0;
            return regs;
        }
    };

    void vehiclePlaceWater(registers& regs, const Flags flags);
}
