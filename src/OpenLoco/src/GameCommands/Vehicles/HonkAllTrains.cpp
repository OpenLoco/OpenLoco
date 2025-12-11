#include "HonkAllTrains.h"
#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"

namespace OpenLoco::GameCommands
{
    static uint32_t honkAllTrains(uint8_t flags)
    {
        if ((flags & Flags::apply) == 0)
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

    void honkAllTrains(registers& regs)
    {
        regs.ebx = honkAllTrains(regs.bl);
    }
}
