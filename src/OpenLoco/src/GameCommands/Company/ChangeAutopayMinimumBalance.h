#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ChangeAutopayMinimumBalanceArgs
    {
        static constexpr auto command = GameCommand::changeAutopayMinBalance;
        ChangeAutopayMinimumBalanceArgs() = default;
        explicit ChangeAutopayMinimumBalanceArgs(const registers& regs)
            : newMinimumBalance(regs.edx)
        {
        }

        currency32_t newMinimumBalance;

        explicit operator registers() const
        {
            registers regs;
            regs.edx = newMinimumBalance;
            return regs;
        }
    };

    void changeAutopayMinBalance(registers& regs);
}
