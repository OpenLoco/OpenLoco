#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleCloneArgs
    {
        static constexpr auto command = GameCommand::vehicleClone;

        VehicleCloneArgs() = default;
        explicit VehicleCloneArgs(const registers& regs)
            : vehicleHeadId(static_cast<EntityId>(regs.ax))
        {
        }

        EntityId vehicleHeadId;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = enumValue(vehicleHeadId);
            return regs;
        }
    };

    void cloneVehicle(registers& regs);
}
