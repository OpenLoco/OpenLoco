#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/Numeric.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004B0B50
    static uint32_t vehicleRefit(const VehicleRefitArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        try
        {
            Vehicles::Vehicle train(args.head);
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
            auto& vehBody = *carIter;

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            uint16_t maxPrimaryCargo = vehObj->maxCargo[0];
            auto cargoTypes = vehObj->cargoTypes[0];
            auto primaryCargoId = Utility::bitScanForward(cargoTypes);
            uint16_t maxCargoUnits = Vehicles::getNumUnitsForCargo(maxPrimaryCargo, primaryCargoId, args.cargoType);

            vehBody.body->primaryCargo.type = args.cargoType;
            vehBody.body->primaryCargo.maxQty = std::min<uint8_t>(maxCargoUnits, 0xFF);
            vehBody.body->primaryCargo.qty = 0;

            auto primaryCargoObj = ObjectManager::get<CargoObject>(primaryCargoId);
            auto acceptedTypes = 0;
            for (uint16_t cargoId = 0; cargoId < ObjectManager::getMaxObjects(ObjectType::cargo); cargoId++)
            {
                auto cargoObject = ObjectManager::get<CargoObject>(cargoId);
                if (cargoObject == nullptr)
                {
                    continue;
                }

                if (cargoObject->matchFlags == primaryCargoObj->matchFlags)
                {
                    acceptedTypes |= 1 << cargoId;
                }
            }
            vehBody.body->primaryCargo.acceptedTypes = acceptedTypes;

            head->sub_4B7CC3();
            Ui::WindowManager::invalidate(Ui::WindowType::vehicle, static_cast<Ui::WindowNumber_t>(head->id));

            return 0;
        }
        catch (std::runtime_error&)
        {
            return FAILURE;
        }
    }

    void vehicleRefit(registers& regs)
    {
        regs.ebx = vehicleRefit(VehicleRefitArgs(regs), regs.bl);
    }
}
