#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RenameTownArgs
    {
        static constexpr auto command = GameCommand::renameTown;

        RenameTownArgs() = default;
        explicit RenameTownArgs(const registers& regs)
            : townId(TownId(regs.cx))
            , nameBufferIndex(regs.ax)
            , buffer{}
        {
            std::memcpy(buffer, &regs.edx, 4);
            std::memcpy(buffer + 4, &regs.ebp, 4);
            std::memcpy(buffer + 8, &regs.edi, 4);
        }

        TownId townId;
        uint8_t nameBufferIndex;
        char buffer[37];

        explicit operator registers() const
        {
            registers regs;

            regs.cx = enumValue(townId);
            regs.ax = nameBufferIndex;
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[nameBufferIndex];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);

            return regs;
        }
    };

    void renameTown(registers& regs);
}
