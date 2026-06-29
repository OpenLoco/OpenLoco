#include "GameCommands/Vehicles/VehiclePassSignal.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle1.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleHead.h"

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
            if (!checkCompanyCompatibility(head->owner))
            {
                return kFailure;
            }

            if (!head->canBeModified())
            {
                return kFailure;
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
            return kFailure;
        }
    }

    void vehiclePassSignal(registers& regs, const uint8_t flags)
    {
        VehiclePassSignalArgs args(regs);
        regs.ebx = vehiclePassSignal(args.head, flags);
    }
}
