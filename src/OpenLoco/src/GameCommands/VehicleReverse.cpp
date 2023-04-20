#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004ADAA8
    static uint32_t vehicleReverse(EntityId headId, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        auto* head = EntityManager::get<Vehicles::VehicleHead>(headId);
        if (head == nullptr)
        {
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            uint8_t var_52_backup = head->var_52;
            head->var_52 = 1;
            head->sub_4ADB47(0);
            head->var_52 = var_52_backup;
            return 0;
        }

        if (!sub_431E6A(head->owner))
        {
            return FAILURE;
        }

        auto* head2 = EntityManager::get<Vehicles::VehicleHead>(head->head);
        if (!head2->canBeModified())
        {
            return FAILURE;
        }

        if (static_cast<uint16_t>(head2->tileX) == 0xFFFF)
        {
            setErrorText(StringIds::empty);
            return FAILURE;
        }

        setPosition(head->position);
        return 0;
    }

    void vehicleReverse(registers& regs)
    {
        regs.ebx = vehicleReverse(EntityId(regs.dx), regs.bl);
    }
}
