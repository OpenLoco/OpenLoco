#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TrackRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTrack;

        TrackRemovalArgs() = default;
        explicit TrackRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh)
            , trackObjectId(regs.ebp)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t trackObjectId;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.ebp = trackObjectId;
            return regs;
        }
    };
}
