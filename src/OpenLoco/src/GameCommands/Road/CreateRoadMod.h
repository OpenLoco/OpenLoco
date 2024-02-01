#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RoadModsPlacementArgs
    {
        static constexpr auto command = GameCommand::createRoadMod;

        RoadModsPlacementArgs() = default;
        explicit RoadModsPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , type((regs.edi >> 16) & 0xF)
            , roadObjType(regs.ebp & 0xFF)
            , modSection((regs.ebp >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t type;
        uint8_t roadObjType;
        uint8_t modSection;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16);
            regs.ebp = roadObjType | (modSection << 16);
            return regs;
        }
    };
}
