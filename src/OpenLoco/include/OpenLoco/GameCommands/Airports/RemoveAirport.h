#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct AirportRemovalArgs
    {
        static constexpr auto command = GameCommand::removeAirport;

        AirportRemovalArgs() = default;
        explicit AirportRemovalArgs(const registers& regs)
            : pos(regs.eax, regs.ecx, regs.edi)
        {
        }

        // Note: pos.z must be a floored BigZ
        World::Pos3 pos;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.edi = pos.z;
            return regs;
        }
    };

    void removeAirport(registers& regs, const Flags flags);
}
