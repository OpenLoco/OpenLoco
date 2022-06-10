#include "../Entities/EntityManager.h"
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
            for (auto& el : tile)
            {
                auto* elStation = el.as<Map::StationElement>();
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

        train.head->tileX = -1;
        train.head->invalidateSprite();
        train.head->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
        train.veh1->tileX = -1;
        train.veh1->invalidateSprite();
        train.veh1->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
        train.veh2->tileX = -1;
        train.veh2->invalidateSprite();
        train.veh2->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
        for (auto& car : train.cars)
        {
            for (auto& carComponent : car)
            {
                carComponent.front->tileX = -1;
                carComponent.front->invalidateSprite();
                carComponent.front->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
                carComponent.back->tileX = -1;
                carComponent.back->invalidateSprite();
                carComponent.back->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
                carComponent.body->tileX = -1;
                carComponent.body->invalidateSprite();
                carComponent.body->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });
            }
        }
        train.tail->tileX = -1;
        train.tail->invalidateSprite();
        train.tail->moveTo({ static_cast<int16_t>(0x8000), 0, 0 });

        train.head->status = Status::unk_0;

        // Clear ghost flag on primary vehicle pieces and all car components.
        train.head->var_38 &= ~(Vehicles::Flags38::isGhost);
        train.veh1->var_38 &= ~(Vehicles::Flags38::isGhost);
        train.veh2->var_38 &= ~(Vehicles::Flags38::isGhost);
        train.tail->var_38 &= ~(Vehicles::Flags38::isGhost);

        for (auto& car : train.cars)
        {
            for (auto& component : car)
            {
                component.front->var_38 &= ~(Vehicles::Flags38::isGhost);
                component.back->var_38 &= ~(Vehicles::Flags38::isGhost);
                component.body->var_38 &= ~(Vehicles::Flags38::isGhost);
            }
        }

        train.head->var_0C |= Vehicles::Flags0C::commandStop;
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
