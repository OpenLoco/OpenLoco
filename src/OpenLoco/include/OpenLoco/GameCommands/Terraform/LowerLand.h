#pragma once

#include "GameCommands/GameCommands.h"
#include "Map/MapSelection.h"
#include "Map/Tile.h"
#include "Map/TileClearance.h"

namespace OpenLoco::GameCommands
{
    struct LowerLandArgs
    {
        static constexpr auto command = GameCommand::lowerLand;
        LowerLandArgs() = default;
        explicit LowerLandArgs(const registers& regs)
            : centre(regs.eax, regs.ecx)
            , pointA(regs.edx, regs.ebp)
            , pointB(regs.esi, regs.edi)
            , corner(static_cast<World::MapSelectionType>(regs.bh))
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        World::MapSelectionType corner;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = centre.x;
            regs.ecx = centre.y;
            regs.edx = pointA.x;
            regs.ebp = pointA.y;
            regs.esi = pointB.x;
            regs.edi = pointB.y;
            regs.bh = enumValue(corner);
            return regs;
        }
    };

    uint32_t lowerLand(const LowerLandArgs& args, World::TileClearance::RemovedBuildings& removedBuildings, const Flags flags);
    void lowerLand(registers& regs, const Flags flags);
}
