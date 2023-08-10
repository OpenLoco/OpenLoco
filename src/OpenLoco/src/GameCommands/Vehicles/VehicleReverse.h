#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleReverseArgs
    {
        static constexpr auto command = GameCommand::vehicleReverse;

        VehicleReverseArgs() = default;
        explicit VehicleReverseArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.dx))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.dx = enumValue(head);
            return regs;
        }
    };

    void vehicleReverse(registers& regs);
}
