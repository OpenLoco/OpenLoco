#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleOrderShareArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderShare;

        VehicleOrderShareArgs() = default;
        explicit VehicleOrderShareArgs(const registers& regs)
            : head(EntityId(regs.di))
            , sourceHead(EntityId(regs.si))
        {
        }

        EntityId head;
        EntityId sourceHead; // null means to detach the orders

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.si = enumValue(sourceHead);
            return regs;
        }
    };

    void vehicleOrderShare(registers& regs, const Flags flags);
}
