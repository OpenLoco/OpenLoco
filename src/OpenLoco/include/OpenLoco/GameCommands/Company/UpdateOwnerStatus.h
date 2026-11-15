#pragma once

#include "GameCommands/GameCommands.h"
#include "World/Company.h"

namespace OpenLoco::GameCommands
{
    struct UpdateOwnerStatusArgs
    {
        static constexpr auto command = GameCommand::updateOwnerStatus;
        UpdateOwnerStatusArgs() = default;
        explicit UpdateOwnerStatusArgs(const registers& regs)
            : ownerStatus(regs.eax, regs.ecx)
        {
        }

        OwnerStatus ownerStatus;

        explicit operator registers() const
        {
            registers regs;
            coord_t res[2];
            ownerStatus.getData(res);
            regs.eax = res[0];
            regs.ecx = res[1];
            return regs;
        }
    };

    void updateOwnerStatus(registers& regs, const Flags flags);
}
