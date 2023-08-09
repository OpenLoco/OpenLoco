#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TownPlacementArgs
    {
        static constexpr auto command = GameCommand::createTown;

        TownPlacementArgs() = default;
        explicit TownPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , size(regs.dl)
        {
        }

        World::Pos2 pos;
        uint8_t size;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edx = size;
            return regs;
        }
    };

    void createTown(registers& regs);
}
