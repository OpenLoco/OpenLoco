#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct LowerRaiseLandMountainArgs
    {
        static constexpr auto command = GameCommand::lowerRaiseLandMountain;
        LowerRaiseLandMountainArgs() = default;
        explicit LowerRaiseLandMountainArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.dx, regs.bp)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , adjustment(regs.di)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        int16_t adjustment;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = (pointB.x << 16) | pointA.x;
            regs.ebp = (pointB.y << 16) | pointA.y;
            regs.di = adjustment;
            return regs;
        }
    };

    void registerMountainHooks();
    void sub_4633F6(World::Pos2 pos);
    void sub_4634B9(World::Pos2 pos);
    void sub_46357C(World::Pos2 pos);
    void sub_46363F(World::Pos2 pos);
}
