#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct IndustryRemovalArgs
    {
        static constexpr auto command = GameCommand::removeIndustry;

        IndustryRemovalArgs() = default;
        explicit IndustryRemovalArgs(const registers& regs)
            : industryId(static_cast<IndustryId>(regs.dl))
        {
        }

        IndustryId industryId;

        explicit operator registers() const
        {
            registers regs;
            regs.dl = enumValue(industryId);
            return regs;
        }
    };

    void removeIndustry(registers& regs);
}
