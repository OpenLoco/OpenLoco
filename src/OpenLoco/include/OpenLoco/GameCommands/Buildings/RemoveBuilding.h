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
            : pos(regs.eax, regs.ecx, regs.edi)
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
            regs.eax = pos.x;
            regs.ecx = pos.y;
            regs.edi = pos.z;
            return regs;
        }
    };

    void removeBuilding(registers& regs, const Flags flags);
}
