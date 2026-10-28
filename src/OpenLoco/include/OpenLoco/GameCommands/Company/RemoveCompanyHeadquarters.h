#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct HeadquarterRemovalArgs
    {
        static constexpr auto command = GameCommand::removeCompanyHeadquarters;

        HeadquarterRemovalArgs() = default;
        explicit HeadquarterRemovalArgs(const World::Pos3& place)
            : pos(place)
        {
        }
        explicit HeadquarterRemovalArgs(const registers& regs)
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

    void removeCompanyHeadquarters(registers& regs, const Flags flags);
}
