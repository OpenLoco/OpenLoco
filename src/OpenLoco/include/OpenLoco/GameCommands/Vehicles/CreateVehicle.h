#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleCreateArgs
    {
        static constexpr auto command = GameCommand::vehicleCreate;

        VehicleCreateArgs() = default;
        explicit VehicleCreateArgs(const registers& regs)
            : vehicleId(static_cast<EntityId>(regs.di))
            , vehicleType(regs.dx)
        {
        }

        EntityId vehicleId; // Optional id representing where it will attach
        uint16_t vehicleType;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(vehicleId);
            regs.edx = vehicleType;
            return regs;
        }
    };

    void createVehicle(registers& regs);
}
