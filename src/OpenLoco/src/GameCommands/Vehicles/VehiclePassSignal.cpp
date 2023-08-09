#include "VehiclePassSignal.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004B0B50
    static uint32_t vehiclePassSignal(EntityId headId, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        try
        {
            Vehicles::Vehicle train(headId);
            setPosition(train.veh2->position);

            auto& head = train.head;
            if (!sub_431E6A(head->owner))
            {
                return FAILURE;
            }

            if (!head->canBeModified())
            {
                return FAILURE;
            }

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            if (head->tileX == -1)
            {
                return 0;
            }

            head->var_5C = 2;
            train.veh1->var_48 |= Vehicles::Flags48::passSignal;
            return 0;
        }
        catch (std::runtime_error&)
        {
            return FAILURE;
        }
    }

    void vehiclePassSignal(registers& regs)
    {
        VehiclePassSignalArgs args(regs);
        regs.ebx = vehiclePassSignal(args.head, regs.bl);
    }
}
