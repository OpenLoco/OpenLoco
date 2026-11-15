#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct PortRemovalArgs
    {
        static constexpr auto command = GameCommand::removePort;

        PortRemovalArgs() = default;
        explicit PortRemovalArgs(const registers& regs)
            : pos(regs.eax, regs.ecx, regs.edi)
        {
        }

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

    void removePort(registers& regs, const Flags flags);
}
