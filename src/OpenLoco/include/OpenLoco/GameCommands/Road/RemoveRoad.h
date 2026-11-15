#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct RoadRemovalArgs
    {
        static constexpr auto command = GameCommand::removeRoad;

        RoadRemovalArgs() = default;
        explicit RoadRemovalArgs(const registers& regs)
            : pos(regs.eax, regs.ecx, regs.edi)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , sequenceIndex(regs.dh & 0x3)
            , objectId(regs.bp & 0xF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t sequenceIndex;
        uint8_t objectId;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.edi = pos.z;
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = sequenceIndex;
            regs.bp = objectId;
            return regs;
        }
    };

    void removeRoad(registers& regs, const Flags flags);
}
