#include "VehiclePickupAir.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Vehicles/Vehicle.h"
#include "World/StationManager.h"

using namespace OpenLoco::Vehicles;

namespace OpenLoco::GameCommands
{
    // 0x00426B29
    static uint32_t vehiclePickupAir(const VehiclePickupAirArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::AircraftRunningCosts);
        Vehicle train(args.head);
        setPosition(train.veh2->position);
        if (!sub_431E6A(train.head->owner))
        {
            return FAILURE;
        }

        if (!train.head->canBeModified())
        {
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        if (train.head->tileX != -1)
        {
            if (!(flags & Flags::ghost))
                Vehicles::playPickupSound(train.veh2);

            train.applyToComponents([](auto& component) {
                component.tileX = -1;
                component.invalidateSprite();
                component.moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
            });

            if (train.head->stationId != StationId::null)
            {
                auto* station = StationManager::get(train.head->stationId);
                station->airportMovementOccupiedEdges &= ~(1ULL << train.head->airportMovementEdge);
            }
        }

        train.head->status = Vehicles::Status::unk_0;
        train.head->stationId = StationId::null;
        train.head->vehicleFlags |= VehicleFlags::commandStop;

        // Clear ghost flag on primary vehicle pieces and all car components.
        train.applyToComponents([](auto& component) {
            component.var_38 &= ~(Vehicles::Flags38::isGhost);
        });

        for (auto& car : train.cars)
        {
            for (auto& component : car)
            {
                removeAllCargo(component);
            }
        }

        return 0;
    }

    void vehiclePickupAir(Interop::registers& regs)
    {
        const VehiclePickupAirArgs args(regs);
        regs.ebx = vehiclePickupAir(args, regs.bl);
    }
}
