#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ClearLandArgs
    {
        static constexpr auto command = GameCommand::clearLand;
        ClearLandArgs() = default;
        explicit ClearLandArgs(const registers& regs)
            : centre(regs.eax, regs.ecx)
            , pointA(regs.edx, regs.ebp)
            , pointB(regs.esi, regs.edi)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = centre.x;
            regs.ecx = centre.y;
            regs.edx = pointA.x;
            regs.ebp = pointA.y;
            regs.esi = pointB.x;
            regs.edi = pointB.y;
            return regs;
        }
    };

    void clearLand(registers& regs, const Flags flags);
}
