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

    struct GenericCheatArgs
    {
        static constexpr auto command = GameCommand::cheat;

        GenericCheatArgs() = default;
        explicit GenericCheatArgs(const registers& regs)
            : subcommand(static_cast<CheatCommand>(regs.ax))
            , param1(regs.ebx)
            , param2(regs.ecx)
            , param3(regs.edx)
        {
        }

        CheatCommand subcommand{};
        int32_t param1{};
        int32_t param2{};
        int32_t param3{};

        explicit operator registers() const
        {
            registers regs;
            regs.ax = enumValue(subcommand);
            regs.ebx = param1;
            regs.ecx = param2;
            regs.edx = param3;
            return regs;
        }
    };

    void vehicleShuntCheat(registers& regs);
    void cheat(registers& regs);
    void freeCashCheat(registers& regs);
}
