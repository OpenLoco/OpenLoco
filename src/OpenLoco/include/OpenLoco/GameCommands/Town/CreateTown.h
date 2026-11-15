#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TownPlacementArgs
    {
        static constexpr auto command = GameCommand::createTown;

        TownPlacementArgs() = default;
        explicit TownPlacementArgs(const registers& regs)
            : pos(regs.eax, regs.ecx)
            , size(regs.dl)
        {
        }

        World::Pos2 pos;
        uint8_t size;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.edx = size;
            return regs;
        }
    };

    void createTown(registers& regs, const Flags flags);
}
