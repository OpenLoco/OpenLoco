#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleRenameArgs
    {
        static constexpr auto command = GameCommand::vehicleRename;

        VehicleRenameArgs() = default;
        explicit VehicleRenameArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.cx))
            , buffer{}
            , i(regs.ax)
        {
            // Copies it into the first 12 bytes not into the specific slot as per i
            std::memcpy(buffer, &regs.edx, 4);
            std::memcpy(buffer + 4, &regs.ebp, 4);
            std::memcpy(buffer + 8, &regs.edi, 4);
        }

        EntityId head;
        char buffer[37];
        uint16_t i;

        explicit operator registers() const
        {
            registers regs;
            regs.cx = enumValue(head);
            regs.ax = i;
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[i];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);
            return regs;
        }
    };

    void renameVehicle(registers& regs);
}
