#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    enum class CheatCommand : uint8_t
    {
        acquireAssets,
        addCash,
        clearLoan,
        companyRatings,
        switchCompany,
        toggleBankruptcy,
        toggleJail,
        vehicleReliability,
        modifyDate,
        completeChallenge,
    };

    void vehicleShuntCheat(registers& regs);
    void cheat(registers& regs);
    void freeCashCheat(registers& regs);
}
