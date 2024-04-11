#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleAirPlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlaceAir;

        VehicleAirPlacementArgs() = default;
        explicit VehicleAirPlacementArgs(const registers& regs)
            : stationId(StationId(regs.bp))
            , airportNode(regs.dl)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx & 0xFFFF0000) == 0xFFFF0000)
        {
        }

        StationId stationId;
        uint8_t airportNode;
        EntityId head;
        bool convertGhost = false;

        explicit operator registers() const
        {
            registers regs;
            regs.bp = enumValue(stationId);
            regs.di = enumValue(head);
            regs.dl = airportNode;
            regs.ebx = convertGhost ? 0xFFFF0000 : 0;
            return regs;
        }
    };

    void vehiclePlaceAir(registers& regs);
}
