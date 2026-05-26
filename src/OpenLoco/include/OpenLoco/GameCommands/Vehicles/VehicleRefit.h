#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleRefitArgs
    {
        static constexpr auto command = GameCommand::vehicleRefit;

        VehicleRefitArgs() = default;
        explicit VehicleRefitArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.di))
            , cargoType(regs.dl)
        {
        }

        EntityId head;
        uint8_t cargoType;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.dl = cargoType;
            return regs;
        }
    };

    void vehicleRefit(registers& regs);
}
