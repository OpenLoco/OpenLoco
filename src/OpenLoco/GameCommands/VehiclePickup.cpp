#include "../Audio/Audio.h"
#include "../Interop/Interop.hpp"
#include "../Management/Expenditures.h"
#include "../Types.hpp"
#include "../Utility/Prng.hpp"
#include "../Vehicles/Vehicle.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<Utility::prng, 0x00525E20> _prng;

    // 0x0048B15B
    static void playPickupSound(Vehicles::Vehicle2* veh2)
    {
        const auto frequency = _prng->randNext(20003, 24098);
        Audio::playSound(Audio::SoundId::vehiclePickup, veh2->position, -1000, frequency);
    }

    // 0x004B0826
    static uint32_t vehiclePickup(const uint8_t flags, EntityId_t headId)
    {
        GameCommands::setExpenditureType(ExpenditureType::TrainRunningCosts);

        auto train = Vehicles::Vehicle(headId);
        auto* head = train.head;
        auto* veh2 = train.veh2;

        GameCommands::setPosition(veh2->position);

        if (!GameCommands::sub_431E6A(head->owner))
            return FAILURE;

        if (!head->canBeModified())
            return FAILURE;

        if (!(flags & GameCommands::Flags::apply))
            return 0;

        if (!(flags & GameCommands::Flags::flag_6))
            playPickupSound(veh2);

        head->liftUpVehicle();

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

        head->var_0C |= Vehicles::Flags0C::commandStop;

        return 0;
    }

    void vehiclePickup(registers& regs)
    {
        regs.ebx = vehiclePickup(regs.bl, regs.di);
    }
}
