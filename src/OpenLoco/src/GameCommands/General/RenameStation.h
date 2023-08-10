#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RenameStationArgs
    {
        static constexpr auto command = GameCommand::changeStationName;

        RenameStationArgs() = default;
        explicit RenameStationArgs(const registers& regs)
            : stationId(StationId(regs.cx))
            , nameBufferIndex(regs.ax)
            , buffer{}
        {
            std::memcpy(buffer, &regs.edx, 4);
            std::memcpy(buffer + 4, &regs.ebp, 4);
            std::memcpy(buffer + 8, &regs.edi, 4);
        }

        StationId stationId;
        uint8_t nameBufferIndex;
        char buffer[37];

        explicit operator registers() const
        {
            registers regs;

            regs.cx = enumValue(stationId);
            regs.ax = nameBufferIndex;
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[nameBufferIndex];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);

            return regs;
        }
    };

    void renameStation(registers& regs);
}
