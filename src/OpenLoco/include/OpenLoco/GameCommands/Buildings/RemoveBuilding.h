#pragma once

#include "GameCommands/Buildings/CreateBuilding.h"
#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct BuildingRemovalArgs
    {
        static constexpr auto command = GameCommand::removeBuilding;

        BuildingRemovalArgs() = default;
        explicit BuildingRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
        {
        }
        explicit BuildingRemovalArgs(const BuildingPlacementArgs& place)
            : pos(place.pos)
        {
        }

        World::Pos3 pos;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            return regs;
        }
    };

    void removeBuilding(registers& regs);
}
