#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleRefreshLocoArgs
    {
        static constexpr auto command = GameCommand::vehicleRefreshLoco;

        VehicleRefreshLocoArgs() = default;
        explicit VehicleRefreshLocoArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.ax))
            //, cargoType(regs.dl)
        {
        }

        EntityId head;
        // uint8_t cargoType;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            //regs.dl = cargoType;
            return regs;
        }
    };

    void vehicleRefreshLoco(registers& regs, const Flags flags);
}
