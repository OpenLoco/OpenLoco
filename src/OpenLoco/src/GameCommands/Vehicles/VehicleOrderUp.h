#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleOrderUpArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderUp;

        VehicleOrderUpArgs() = default;
        explicit VehicleOrderUpArgs(const registers& regs)
            : head(EntityId(regs.di))
            , orderOffset(regs.edx)
        {
        }

        EntityId head;
        uint32_t orderOffset;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.edx = orderOffset;
            return regs;
        }
    };

    void vehicleOrderUp(registers& regs);
}
