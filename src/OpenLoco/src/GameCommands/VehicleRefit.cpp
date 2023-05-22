#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/Numeric.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004B0B50
    static uint32_t vehicleRefit(EntityId headId, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        try
        {
            Vehicles::Vehicle train(headId);
            auto& head = train.head;

            if (!head->canBeModified())
            {
                return FAILURE;
            }

            if (!sub_431E6A(head->owner))
            {
                return FAILURE;
            }

            if (train.cars.empty())
            {
                setErrorText(StringIds::empty);
                return 0;
            }

            auto car = train.cars.firstCar;
            auto* vehObj = ObjectManager::get<VehicleObject>(car.body->objectId);

            if (!vehObj->hasFlags(VehicleObjectFlags::refittable))
            {
                setErrorText(StringIds::empty);
                return 0;
            }

            auto carIter = train.cars.begin();
            carIter++;
            car = *carIter;

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            auto maxPrimaryCargo = vehObj->maxCargo[0];
            auto cargoTypes = vehObj->cargoTypes[0];
            auto primaryCargoId = Utility::bitScanForward(cargoTypes);

            // TODO: check invocation -- not implemented with primaryCargoId == cargoId in mind, perhaps?
            // auto units = Vehicles::getNumUnitsForCargo(maxPrimaryCargo, primaryCargoId, primaryCargoId);

            // TODO: finish

            return 0;
        }
        catch (std::runtime_error&)
        {
            return FAILURE;
        }
    }

    void vehicleRefit(registers& regs)
    {
        VehiclePassSignalArgs args(regs);
        regs.ebx = vehicleRefit(args.head, regs.bl);
    }
}
