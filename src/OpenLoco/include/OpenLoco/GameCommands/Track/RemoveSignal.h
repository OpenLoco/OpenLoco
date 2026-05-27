#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct SignalRemovalArgs
    {
        static constexpr auto command = GameCommand::removeSignal;

        SignalRemovalArgs() = default;
        explicit SignalRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0xF)
            , trackObjType(regs.bp & 0xF)
            , flags(regs.edi >> 16)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t trackObjType;
        uint16_t flags;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (flags << 16);
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.bp = trackObjType;
            return regs;
        }
    };

    void removeSignal(registers& regs);
}
