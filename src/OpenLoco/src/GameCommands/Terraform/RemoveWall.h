#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct WallRemovalArgs
    {
        static constexpr auto command = GameCommand::removeWall;

        WallRemovalArgs() = default;
        explicit WallRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dh * World::kSmallZStep)
            , rotation(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dh = pos.z / World::kSmallZStep;
            regs.dl = rotation;
            return regs;
        }
    };

    void removeWall(registers& regs);
}
