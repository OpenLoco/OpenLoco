#include "GameCommands/Vehicles/HonkAllTrains.h"
#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"

namespace OpenLoco::GameCommands
{
    static uint32_t honkAllTrains(Flags flags)
    {
        if ((flags & Flags::apply) == Flags::none)
        {
            return 0;
        }

        for (auto* head : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*head);
            railProduceCrossingWhistle(*train.veh2);
        }

        return 0;
    }

    void honkAllTrains(registers& regs, Flags flags)
    {
        regs.ebx = honkAllTrains(flags);
    }
}
