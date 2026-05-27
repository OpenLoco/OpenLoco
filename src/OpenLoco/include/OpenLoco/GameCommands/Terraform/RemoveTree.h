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
            , elementType(regs.bh)
        {
        }

        World::Pos3 pos;
        uint8_t type;
        uint8_t elementType;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = pos.z / World::kSmallZStep;
            regs.dh = type;
            regs.bh = elementType;
            return regs;
        }
    };

    void removeTree(registers& regs);
}
