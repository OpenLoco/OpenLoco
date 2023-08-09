#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleOrderReverseArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderReverse;

        VehicleOrderReverseArgs() = default;
        explicit VehicleOrderReverseArgs(const registers& regs)
            : head(EntityId(regs.di))
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

    void vehicleOrderReverse(registers& regs);
}
