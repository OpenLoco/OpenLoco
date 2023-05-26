#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Vehicles/Vehicle.h"

using namespace OpenLoco::Vehicles;

namespace OpenLoco::GameCommands
{
    // 0x004279CC
    static uint32_t vehiclePickupWater(EntityId head, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::ShipRunningCosts);
        Vehicle train(head);
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

        if (!(flags & Flags::flag_6))
            Vehicles::playPickupSound(train.veh2);

        if (train.head->stationId != StationId::null)
        {
            auto tile = World::TileManager::get(train.head->getTrackLoc());
            for (auto& el : tile)
            {
                auto* elStation = el.as<World::StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }
                if (elStation->baseZ() != train.head->tileBaseZ)
                {
                    continue;
                }

                if (elStation->isGhost() || elStation->isFlag5())
                {
                    continue;
                }

                elStation->setFlag6(false);
                train.head->stationId = StationId::null;
            }
        }

        train.applyToComponents([](auto& component) {
            component.tileX = -1;
            component.invalidateSprite();
            component.moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
        });

        train.head->status = Status::unk_0;

        // Clear ghost flag on primary vehicle pieces and all car components.
        train.applyToComponents([](auto& component) {
            component.var_38 &= ~(Vehicles::Flags38::isGhost);
        });

        train.head->vehicleFlags |= VehicleFlags::commandStop;
        for (auto& car : train.cars)
        {
            for (auto& component : car)
            {
                component.front->carComponent_sub_4AF16A();
            }
        }
        return 0;
    }

    void vehiclePickupWater(Interop::registers& regs)
    {
        regs.ebx = vehiclePickupWater(EntityId(regs.di), regs.bl);
    }
}
