#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands.h"
#include "Random.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Core/Prng.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x0048B15B
    void playPickupSound(Vehicles::Vehicle2* veh2)
    {
        const auto frequency = gPrng2().randNext(20003, 24098);
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

        head->vehicleFlags |= VehicleFlags::commandStop;

        return 0;
    }

    void vehiclePickup(registers& regs)
    {
        regs.ebx = vehiclePickup(regs.bl, EntityId(regs.di));
    }
}
