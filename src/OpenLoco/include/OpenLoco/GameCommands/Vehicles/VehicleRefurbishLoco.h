#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleRefurbishLocoArgs
    {
        static constexpr auto command = GameCommand::vehicleRefurbishLoco;

        VehicleRefurbishLocoArgs() = default;
        explicit VehicleRefurbishLocoArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.ax))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = enumValue(head);
            // regs.dl = cargoType;
            return regs;
        }
    };

    void vehicleRefurbishLoco(registers& regs, const Flags flags);
}
