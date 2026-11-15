#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct AirportPlacementArgs
    {
        static constexpr auto command = GameCommand::createAirport;

        AirportPlacementArgs() = default;
        explicit AirportPlacementArgs(const registers regs)
            : pos(regs.eax, regs.ecx, regs.edi)
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
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.edi = pos.z;
            regs.bh = rotation;
            regs.dl = type;
            return regs;
        }
    };

    void createAirport(registers& regs, const Flags flags);
}
