#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehiclePickupAirArgs
    {
        static constexpr auto command = GameCommand::vehiclePickupAir;

        VehiclePickupAirArgs() = default;
        explicit VehiclePickupAirArgs(const registers& regs)
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

    void vehiclePickupAir(Interop::registers& regs);
}
