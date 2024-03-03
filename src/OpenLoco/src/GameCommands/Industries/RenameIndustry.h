#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RenameIndustryArgs
    {
        static constexpr auto command = GameCommand::renameTown;

        RenameIndustryArgs() = default;
        explicit RenameIndustryArgs(const registers& regs)
            : industryId(IndustryId(regs.cx))
            , nameBufferIndex(regs.ax)
            , buffer{}
        {
            std::memcpy(buffer, &regs.edx, 4);
            std::memcpy(buffer + 4, &regs.ebp, 4);
            std::memcpy(buffer + 8, &regs.edi, 4);
        }

        IndustryId industryId;
        uint8_t nameBufferIndex;
        char buffer[37];

        explicit operator registers() const
        {
            registers regs;

            regs.cx = enumValue(industryId);
            regs.ax = nameBufferIndex;
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[nameBufferIndex];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);

            return regs;
        }
    };

    void renameIndustry(registers& regs);
}
