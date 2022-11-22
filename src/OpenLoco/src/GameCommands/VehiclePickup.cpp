#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "Interop/Interop.hpp"
#include "Types.hpp"
#include "Utility/Prng.hpp"
#include "Vehicles/Vehicle.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<Utility::Prng, 0x00525E20> _prng;

    // 0x0048B15B
    void playPickupSound(Vehicles::Vehicle2* veh2)
    {
        const auto frequency = _prng->randNext(20003, 24098);
        Audio::playSound(Audio::SoundId::vehiclePickup, veh2->position, -1000, frequency);
    }
}

namespace OpenLoco::GameCommands
{
    // 0x004B0826
    static uint32_t vehiclePickup(const uint8_t flags, EntityId headId)
    {
        GameCommands::setExpenditureType(ExpenditureType::TrainRunningCosts);

        auto* head = EntityManager::get<Vehicles::VehicleHead>(headId);
        if (head == nullptr)
        {
            return FAILURE;
        }
        auto train = Vehicles::Vehicle(*head);
        auto* veh2 = train.veh2;

        GameCommands::setPosition(veh2->position);

        if (!GameCommands::sub_431E6A(head->owner))
            return FAILURE;

        if (!head->canBeModified())
            return FAILURE;

        if (!(flags & GameCommands::Flags::apply))
            return 0;

        if (!(flags & GameCommands::Flags::flag_6))
            Vehicles::playPickupSound(veh2);

        head->liftUpVehicle();

        // Clear ghost flag on primary vehicle pieces and all car components.
        train.applyToComponents([](auto& component) { component.var_38 &= ~Vehicles::Flags38::isGhost; });

        head->var_0C |= Vehicles::Flags0C::commandStop;

        return 0;
    }

    void vehiclePickup(registers& regs)
    {
        regs.ebx = vehiclePickup(regs.bl, EntityId(regs.di));
    }
}
