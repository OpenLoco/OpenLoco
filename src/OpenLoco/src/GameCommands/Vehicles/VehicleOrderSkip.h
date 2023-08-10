#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleOrderSkipArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderSkip;

        VehicleOrderSkipArgs() = default;
        explicit VehicleOrderSkipArgs(const registers& regs)
            : head(EntityId(regs.di))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            return regs;
        }
    };

    void vehicleOrderSkip(registers& regs);
}
