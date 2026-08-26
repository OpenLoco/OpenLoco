#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct SignalsPlacementAutoArgs
    {
        static constexpr auto command = GameCommand::createSignalsAuto;

        SignalsPlacementAutoArgs() = default;
        explicit SignalsPlacementAutoArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh)
            , type((regs.edi >> 16) & 0xFF)
            , trackObjType(regs.ebp & 0xFF)
            , sides((regs.edi >> 16) & 0xC000)
            , step((regs.ecx >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t type;
        uint8_t trackObjType;
        uint16_t sides;
        uint8_t step = 1;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.ecx = pos.y | (step << 16);
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16) | ((sides & 0xC000) << 16);
            regs.ebp = trackObjType;
            return regs;
        }
    };

    void createSignalsAuto(registers& regs, const Flags flags);
}
