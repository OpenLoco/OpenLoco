#pragma once

#include "GameCommands/GameCommands.h"
#include <Map/TileClearance.h>

namespace OpenLoco::GameCommands
{
    struct ClearLandArgs
    {
        static constexpr auto command = GameCommand::clearLand;
        ClearLandArgs() = default;
        explicit ClearLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.edx & 0xFFFF, regs.ebp & 0xFFFF)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , filters(static_cast<World::TileClearance::ClearFilters>(regs.bh))
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        World::TileClearance::ClearFilters filters = (World::TileClearance::ClearFilters::scenery | World::TileClearance::ClearFilters::buildings);

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = (pointB.x << 16) | pointA.x;
            regs.ebp = (pointB.y << 16) | pointA.y;
            regs.bh = static_cast<int8_t>(filters);
            return regs;
        }
    };

    void clearLand(registers& regs, const Flags flags);
}
