#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehiclePickupWaterArgs
    {
        static constexpr auto command = GameCommand::vehiclePickupWater;

        VehiclePickupWaterArgs() = default;
        explicit VehiclePickupWaterArgs(const registers& regs)
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

    void vehiclePickupWater(Interop::registers& regs);
}
