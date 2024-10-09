#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RoadPlacementArgs
    {
        static constexpr auto command = GameCommand::createRoad;

        RoadPlacementArgs() = default;
        explicit RoadPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dh & 0xF)
            , mods(regs.di >> 16)
            , bridge(regs.edx >> 24)
            , roadObjectId(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t mods;
        uint8_t bridge;
        uint8_t roadObjectId;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.cx = pos.y;
            regs.edi = (0xFFFFU & pos.z) | (mods << 16);
            regs.bh = rotation;
            regs.edx = roadObjectId | (roadId << 8) | (bridge << 24);
            return regs;
        }
    };

    void createRoad(registers& regs);
}
