#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct BuildingPlacementArgs
    {
        static constexpr auto command = GameCommand::createBuilding;

        BuildingPlacementArgs() = default;
        explicit BuildingPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , type(regs.dl)
            , variation(regs.dh)
            , colour(static_cast<Colour>((regs.edi >> 16) & 0x1F))
            , buildImmediately(regs.bh & 0x80)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;
        uint8_t variation;
        Colour colour;
        bool buildImmediately = false; // No scaffolding required (editor mode)
        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (enumValue(colour) << 16);
            regs.dl = type;
            regs.dh = variation;
            regs.bh = rotation | (buildImmediately ? 0x80 : 0);
            return regs;
        }
    };
}
