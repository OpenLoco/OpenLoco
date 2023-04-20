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

        if (!sub_431E6A(head->owner))
        {
            return FAILURE;
        }

        if (head->tileX == -1)
        {
            setErrorText(StringIds::empty);
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        uint8_t var_52_backup = head->var_52;
        head->var_52 = 1;
        head->sub_4ADB47(0);
        head->var_52 = var_52_backup;
        setPosition(head->position);

        return 0;
    }

    void vehicleReverse(registers& regs)
    {
        VehicleReverseArgs args(regs);
        regs.ebx = vehicleReverse(args.head, regs.bl);
    }
}
