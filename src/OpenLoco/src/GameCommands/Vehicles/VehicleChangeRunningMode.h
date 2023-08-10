#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleChangeRunningModeArgs
    {
        enum class Mode : uint8_t
        {
            stopVehicle,
            startVehicle,
            toggleLocalExpress,
            driveManually,
        };

        static constexpr auto command = GameCommand::vehicleChangeRunningMode;

        VehicleChangeRunningModeArgs() = default;
        explicit VehicleChangeRunningModeArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.dx))
            , mode(static_cast<Mode>(regs.bh))
        {
        }

        EntityId head;
        Mode mode;

        explicit operator registers() const
        {
            registers regs;
            regs.dx = enumValue(head);
            regs.bh = enumValue(mode);
            return regs;
        }
    };

    void vehicleChangeRunningMode(registers& regs);
}
