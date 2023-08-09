#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct WallPlacementArgs
    {
        static constexpr auto command = GameCommand::createWall;

        WallPlacementArgs() = default;
        explicit WallPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.dl)
            , type(regs.bh)
            , primaryColour(static_cast<Colour>(regs.dh))
            , secondaryColour(static_cast<Colour>(regs.bp & 0x1F))
            , tertiaryColour(static_cast<Colour>((regs.bp >> 8) & 0x1F))
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;
        Colour primaryColour;
        Colour secondaryColour;
        Colour tertiaryColour; // Note: will not work; render engine does not support tertiary

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = rotation;
            regs.dh = enumValue(primaryColour);
            regs.di = pos.z;
            regs.bp = enumValue(secondaryColour) | (enumValue(tertiaryColour) << 8);
            regs.bh = type;
            return regs;
        }
    };

    void createWall(registers& regs);
}
