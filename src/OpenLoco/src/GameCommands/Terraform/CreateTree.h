#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TreePlacementArgs
    {
        static constexpr auto command = GameCommand::createTree;

        TreePlacementArgs() = default;
        explicit TreePlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , rotation(regs.di & 0x3)
            , type(regs.bh)
            , quadrant(regs.dl)
            , colour(static_cast<Colour>(regs.dh & 0x1F))
            , buildImmediately(regs.di & 0x8000)
            , requiresFullClearance(regs.di & 0x4000)
        {
        }

        World::Pos2 pos;
        uint8_t rotation;
        uint8_t type;
        uint8_t quadrant;
        Colour colour;
        bool buildImmediately = false;
        bool requiresFullClearance = false;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = quadrant;
            regs.dh = enumValue(colour);
            regs.di = rotation | (buildImmediately ? 0x8000 : 0) | (requiresFullClearance ? 0x4000 : 0);
            regs.bh = type;
            return regs;
        }
    };

    void createTree(registers& regs);
}
