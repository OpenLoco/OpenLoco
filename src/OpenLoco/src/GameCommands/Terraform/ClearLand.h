#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ClearLandArgs
    {
        static constexpr auto command = GameCommand::clearLand;
        ClearLandArgs() = default;
        explicit ClearLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.edx & 0xFFFF, regs.ebp & 0xFFFF)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = (pointB.x << 16) | pointA.x;
            regs.ebp = (pointB.y << 16) | pointA.y;
            return regs;
        }
    };

    void clearLand(registers& regs);
}
