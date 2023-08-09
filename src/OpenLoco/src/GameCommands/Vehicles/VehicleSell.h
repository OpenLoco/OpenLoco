#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleSellArgs
    {
        static constexpr auto command = GameCommand::vehicleSell;

        VehicleSellArgs() = default;
        explicit VehicleSellArgs(const registers& regs)
            : car(static_cast<EntityId>(regs.dx))
        {
        }

        EntityId car;

        explicit operator registers() const
        {
            registers regs;
            regs.dx = enumValue(car);
            return regs;
        }
    };

    void sellVehicle(Interop::registers& regs);
}
