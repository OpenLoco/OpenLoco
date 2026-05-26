#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehiclePassSignalArgs
    {
        static constexpr auto command = GameCommand::vehiclePassSignal;

        VehiclePassSignalArgs() = default;
        explicit VehiclePassSignalArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.di))
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

    void vehiclePassSignal(registers& regs);
}
