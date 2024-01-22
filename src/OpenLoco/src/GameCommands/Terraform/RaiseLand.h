#pragma once

#include "GameCommands/GameCommands.h"
#include "Map/MapSelection.h"
#include "Map/Tile.h"
#include <set>

namespace OpenLoco::GameCommands
{
    struct RaiseLandArgs
    {
        static constexpr auto command = GameCommand::raiseLand;
        RaiseLandArgs() = default;
        explicit RaiseLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.dx, regs.bp)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , corner(regs.di)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        World::MapSelectionType corner;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = (pointB.x << 16) | pointA.x;
            regs.ebp = (pointB.y << 16) | pointA.y;
            regs.di = enumValue(corner);
            return regs;
        }
    };

    uint32_t raiseLand(const RaiseLandArgs& args, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, const uint8_t flags);
    void raiseLand(registers& regs);
}
