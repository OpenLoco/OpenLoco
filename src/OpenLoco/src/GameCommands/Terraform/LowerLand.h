#pragma once

#include "GameCommands/GameCommands.h"
#include "Map/MapSelection.h"
#include "Map/Tile.h"
#include <set>

namespace OpenLoco::GameCommands
{
    struct LowerLandArgs
    {
        static constexpr auto command = GameCommand::lowerLand;
        LowerLandArgs() = default;
        explicit LowerLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.dx, regs.bp)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , corner(static_cast<World::MapSelectionType>(regs.di))
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

    uint32_t lowerLand(const LowerLandArgs& args, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, const uint8_t flags);
    void lowerLand(registers& regs);
}
