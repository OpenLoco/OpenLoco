#pragma once
#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleRearrangeArgs
    {
        static constexpr auto command = GameCommand::vehicleRearrange;

        VehicleRearrangeArgs() = default;
        explicit VehicleRearrangeArgs(const registers& regs)
            : source(static_cast<EntityId>(regs.dx))
            , dest(static_cast<EntityId>(regs.di))
        {
        }

        EntityId source;
        EntityId dest;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(dest);
            regs.dx = enumValue(source);
            return regs;
        }
    };

    void vehicleRearrange(registers& regs);
}
