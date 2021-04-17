#include "../Interop/Interop.hpp"
#include "../Management/Expenditures.h"
#include "../Types.hpp"
#include "../Vehicles/Vehicle.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0048B15B
    static void playPickupSound()
    {
        auto position = GameCommands::getPosition();
        registers regs;
        regs.cx = position.x;
        regs.dx = position.y;
        regs.bp = position.z;
        call(0x0048B15B, regs);
    }

    static uint32_t vehiclePickup(const uint8_t flags, EntityId_t headId)
    {
        GameCommands::setExpenditureType(ExpenditureType::TrainRunningCosts);

        auto train = Vehicles::Vehicle(headId);
        auto* head = train.head;
        auto* veh2 = train.veh2;

        const auto pos = Map::map_pos3(veh2->x, veh2->y, veh2->z);
        GameCommands::setPosition(pos);

        if (!GameCommands::sub_431E6A(head->owner))
            return FAILURE;

        if (!head->canBeModified())
            return FAILURE;

        if (!(flags & GameCommands::GameCommandFlag::apply))
            return 0;

        if (!(flags & GameCommands::GameCommandFlag::flag_6))
            playPickupSound();

        head->liftUpVehicle();

        for (auto& car : train.cars)
        {
            for (auto& component : car)
            {
                component.front->var_38 &= ~(Vehicles::Flags38::isGhost);
                component.back->var_38 &= ~(Vehicles::Flags38::isGhost);
                component.body->var_38 &= ~(Vehicles::Flags38::isGhost);
            }
        }

        train.tail->var_0C |= Vehicles::Flags0C::commandStop;

        return 0;
    }

    void vehiclePickup(registers& regs)
    {
        regs.ebx = vehiclePickup(regs.bl, regs.di);
    }
}
