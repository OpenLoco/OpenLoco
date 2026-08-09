#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct SignalsRemovalAutoArgs
    {
        static constexpr auto command = GameCommand::removeSignalsAuto;

        SignalsRemovalAutoArgs() = default;
        explicit SignalsRemovalAutoArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0xF)
            , trackObjType(regs.bp & 0xF)
            , flags(regs.edi >> 16)
            , step((regs.ecx >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t trackObjType;
        uint16_t flags;
        uint8_t step;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.ecx = pos.y | (step << 16);
            regs.edi = pos.z | (flags << 16);
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.bp = trackObjType;
            return regs;
        }
    };

    void removeSignalsAuto(registers& regs, const uint8_t flags);
}
