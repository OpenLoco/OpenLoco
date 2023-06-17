#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0042F6DB
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

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            uint16_t maxPrimaryCargo = vehObj->maxCargo[0];
            auto cargoTypes = vehObj->cargoTypes[0];
            auto primaryCargoId = Numerics::bitScanForward(cargoTypes);
            uint16_t maxCargoUnits = Vehicles::getNumUnitsForCargo(maxPrimaryCargo, primaryCargoId, args.cargoType);

            car.body->primaryCargo.type = args.cargoType;
            car.body->primaryCargo.maxQty = std::min<uint8_t>(maxCargoUnits, 0xFF);
            car.body->primaryCargo.qty = 0;

            auto primaryCargoObj = ObjectManager::get<CargoObject>(args.cargoType);
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
            car.body->primaryCargo.acceptedTypes = acceptedTypes;

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
