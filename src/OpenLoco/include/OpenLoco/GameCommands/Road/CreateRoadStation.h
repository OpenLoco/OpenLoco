#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RoadStationPlacementArgs
    {
        static constexpr auto command = GameCommand::createRoadStation;

        RoadStationPlacementArgs() = default;
        explicit RoadStationPlacementArgs(const registers& regs)
            : pos(regs.eax, regs.ecx, regs.edi)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , roadObjectId(regs.bp)
            , type(regs.edi >> 16)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t roadObjectId;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.edi = pos.z | (type << 16);
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = index;
            regs.bp = roadObjectId;
            return regs;
        }
    };

    void createRoadStation(registers& regs, const Flags flags);
}
