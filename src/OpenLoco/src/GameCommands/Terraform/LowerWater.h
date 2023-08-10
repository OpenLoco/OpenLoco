#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct LowerWaterArgs
    {
        static constexpr auto command = GameCommand::lowerWater;
        LowerWaterArgs() = default;
        explicit LowerWaterArgs(const registers& regs)
            : pointA(regs.ax, regs.cx)
            , pointB(regs.di, regs.bp)
        {
        }

        World::Pos2 pointA;
        World::Pos2 pointB;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pointA.x;
            regs.cx = pointA.y;
            regs.di = pointB.x;
            regs.bp = pointB.y;
            return regs;
        }
    };

    void lowerWater(registers& regs);
}
