#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TreeRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTree;

        TreeRemovalArgs() = default;
        explicit TreeRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dl * World::kSmallZStep)
            , type(regs.dh)
            , quadrant(regs.di & 0b11)
            , rotation(regs.bh & 0b11)
        {
        }

        World::Pos3 pos;
        uint8_t type;
        uint8_t quadrant;
        uint8_t rotation;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = pos.z / World::kSmallZStep;
            regs.dh = type;
            regs.bh = rotation & 0b11;
            regs.di = quadrant & 0b11;
            return regs;
        }
    };

    void removeTree(registers& regs, const uint8_t flags);
}
