#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehiclePickupArgs
    {
        static constexpr auto command = GameCommand::vehiclePickup;

        VehiclePickupArgs() = default;
        explicit VehiclePickupArgs(const registers& regs)
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

    void vehiclePickup(registers& regs);
}
