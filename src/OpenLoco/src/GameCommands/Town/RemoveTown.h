#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TownRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTown;

        TownRemovalArgs() = default;
        explicit TownRemovalArgs(const registers& regs)
            : townId(TownId(regs.edi))
        {
        }

        TownId townId;

        explicit operator registers() const
        {
            registers regs;
            regs.edi = enumValue(townId);
            return regs;
        }
    };

    void removeTown(registers& regs);
}
