#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TrackPlacementArgs
    {
        static constexpr auto command = GameCommand::createTrack;

        TrackPlacementArgs() = default;
        explicit TrackPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dh & 0x3F)
            , mods(regs.di >> 16)
            , unkFlags((regs.edx >> 20) & 0xF)
            , bridge((regs.edx >> 24) & 0xFF)
            , trackObjectId(regs.dl)
            , unk(regs.edi & 0x800000)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t mods;
        uint8_t unkFlags;
        uint8_t bridge;
        uint8_t trackObjectId;
        bool unk;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.cx = pos.y;
            regs.edi = (0xFFFF & pos.z) | (mods << 16) | (unk ? 0x800000 : 0);
            regs.bh = rotation;
            regs.edx = trackObjectId | (trackId << 8) | (unkFlags << 20) | (bridge << 24);
            return regs;
        }
    };

    void createTrack(registers& regs);
}
