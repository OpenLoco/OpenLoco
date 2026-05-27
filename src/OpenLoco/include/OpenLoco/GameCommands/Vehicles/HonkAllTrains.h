#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct HonkAllTrainsArgs
    {
        static constexpr auto command = GameCommand::honkAllTrains;

        HonkAllTrainsArgs() = default;
        explicit HonkAllTrainsArgs(const registers&)
        {
        }

        explicit operator registers() const
        {
            return registers();
        }
    };

    void honkAllTrains(registers& regs);
}
