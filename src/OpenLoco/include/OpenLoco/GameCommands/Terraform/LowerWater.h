#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct LowerWaterArgs
    {
        static constexpr auto command = GameCommand::lowerWater;
        LowerWaterArgs() = default;
        explicit LowerWaterArgs(const registers& regs)
            : pointA(regs.eax, regs.ecx)
            , pointB(regs.edi, regs.ebp)
        {
        }

        World::Pos2 pointA;
        World::Pos2 pointB;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pointA.x;
            regs.ecx = pointA.y;
            regs.edi = pointB.x;
            regs.ebp = pointB.y;
            return regs;
        }
    };

    void lowerWater(registers& regs, const Flags flags);
}
