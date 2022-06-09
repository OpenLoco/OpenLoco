#include "../GameCommands/GameCommands.h"
#include "../Map/TileManager.h"
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    // 0x004279CC
    static uint32_t vehiclePickupWater(EntityId head, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::ShipRunningCosts);
        Vehicle train(head);
        GameCommands::setPosition(train.veh2->position);
        if (!GameCommands::sub_431E6A(train.head->owner))
        {
            return GameCommands::FAILURE;
        }

        if (!train.head->canBeModified())
        {
            return GameCommands::FAILURE;
        }

        if (!(flags & GameCommands::Flags::apply))
        {
            return 0;
        }

        if (!(flags & GameCommands::Flags::flag_6))
            Vehicles::playPickupSound(train.veh2);

        if (train.head->stationId != StationId::null)
        {
            auto tile = Map::TileManager::get(train.head->getTrackLoc());
        }
    }

    void vehiclePickupWater(Interop::registers& regs)
    {
        regs.ebx = vehiclePickupWater(EntityId(regs.di), regs.bl);
    }
}
