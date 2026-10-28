#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ChangeLandMaterialArgs
    {
        static constexpr auto command = GameCommand::changeLandMaterial;
        ChangeLandMaterialArgs() = default;
        explicit ChangeLandMaterialArgs(const registers& regs)
            : pointA(regs.eax, regs.ecx)
            , pointB(regs.edi, regs.ebp)
            , landType(regs.dl)
        {
        }

        World::Pos2 pointA;
        World::Pos2 pointB;
        uint8_t landType;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pointA.x;
            regs.ecx = pointA.y;
            regs.edi = pointB.x;
            regs.ebp = pointB.y;
            regs.dl = landType;
            return regs;
        }
    };

    void changeLandMaterial(registers& regs, const Flags flags);
}
