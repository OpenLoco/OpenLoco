#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct AirportPlacementArgs
    {
        static constexpr auto command = GameCommand::createAirport;

        AirportPlacementArgs() = default;
        explicit AirportPlacementArgs(const registers regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh)
            , type(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = type;
            return regs;
        }
    };
}
