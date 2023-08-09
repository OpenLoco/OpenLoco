#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleSpeedControlArgs
    {
        static constexpr auto command = GameCommand::vehicleSpeedControl;

        VehicleSpeedControlArgs() = default;
        explicit VehicleSpeedControlArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.cx))
            , speed(regs.dx)
        {
        }

        EntityId head;
        int16_t speed;

        explicit operator registers() const
        {
            registers regs;
            regs.cx = enumValue(head);
            regs.dx = speed;
            return regs;
        }
    };

    void vehicleSpeedControl(registers& regs);
}
