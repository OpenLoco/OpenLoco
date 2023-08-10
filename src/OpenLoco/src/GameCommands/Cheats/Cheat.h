#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleApplyShuntCheatArgs
    {
        static constexpr auto command = GameCommand::vehicleApplyShuntCheat;

        VehicleApplyShuntCheatArgs() = default;
        explicit VehicleApplyShuntCheatArgs(const registers& regs)
            : head(EntityId(regs.cx))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.cx = enumValue(head);
            return regs;
        }
    };

    struct ApplyFreeCashCheatArgs
    {
        static constexpr auto command = GameCommand::applyFreeCashCheat;

        ApplyFreeCashCheatArgs() = default;
        explicit ApplyFreeCashCheatArgs(const registers&)
        {
        }

        explicit operator registers() const
        {
            return registers();
        }
    };

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
