#pragma once

#include "GameCommands/GameCommands.h"
#include "Map/Tile.h"
#include <set>

namespace OpenLoco::GameCommands
{
    struct LowerRaiseLandMountainArgs
    {
        static constexpr auto command = GameCommand::lowerRaiseLandMountain;
        LowerRaiseLandMountainArgs() = default;
        explicit LowerRaiseLandMountainArgs(const registers& regs)
            : centre(regs.eax, regs.ecx)
            , pointA(regs.edx, regs.ebp)
            , pointB(regs.esi, regs.edi)
            , adjustment(regs.bh)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        int16_t adjustment;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = centre.x;
            regs.ecx = centre.y;
            regs.edx = pointA.x;
            regs.ebp = pointA.y;
            regs.esi = pointB.x;
            regs.edi = pointB.y;
            regs.bh = adjustment;
            return regs;
        }
    };

    void lowerRaiseLandMountain(registers& regs, const Flags flags);
}
