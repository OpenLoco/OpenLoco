#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct SetGameSpeedArgs
    {
        static constexpr auto command = GameCommand::setGameSpeed;
        SetGameSpeedArgs() = default;
        explicit SetGameSpeedArgs(const registers& regs)
            : newSpeed(static_cast<GameSpeed>(regs.edi))
        {
        }

        explicit SetGameSpeedArgs(const GameSpeed speed)
        {
            newSpeed = speed;
        }

        GameSpeed newSpeed;

        explicit operator registers() const
        {
            registers regs;
            regs.edi = static_cast<std::underlying_type_t<GameSpeed>>(newSpeed);
            return regs;
        }
    };

    void setGameSpeed(registers& regs);
}
