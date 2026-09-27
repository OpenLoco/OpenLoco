#include "GameCommands/Vehicles/VehicleRefreshLoco.h"
#include "Date.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
// #include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleBody.h"
#include "Vehicles/VehicleBogie.h"
#include "Vehicles/VehicleHead.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    // 0x0042F6DB
    static uint32_t vehicleRefreshLoco(const VehicleRefreshLocoArgs& args, const Flags flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        try
        {
            Vehicles::Vehicle train(args.head);
            auto& head = train.head;

            if (!head->canBeModified())
            {
                return kFailure;
            }

            if (!checkCompanyCompatibility(head->owner))
            {
                return kFailure;
            }

            /*
            if (train.cars.empty())
            {
                setErrorText(StringIds::empty);
                return 0;
            }
            */

            auto car1 = train.cars.firstCar;
            auto* vehObj1 = ObjectManager::get<VehicleObject>(car1.body->objectId);
            
            currency32_t refundCost = 0;
            currency32_t purchaseCost = 0;
            // auto cost = Economy::getInflationAdjustedCost(vehObject.costFactor, vehObject.costIndex, 6);

            // StringIds::vehicle_is_locked

            for (const auto& car : train.cars)
            {
                auto* vehObj = ObjectManager::get<VehicleObject>(car.body->objectId);
                if (vehObj->power != 0) // check that it's a power car. 
                {
                  auto temp_cost = Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
                  refundCost += car.front->refundCost;
                  purchaseCost += temp_cost;
                  
                }
                // refundCost += car.front->refundCost;
            }

            // Note: Refund cost might already be negative signed;
            // make sure netCost <= purchaseCost.
            currency32_t netCost = purchaseCost - refundCost;
            assert (netCost <= purchaseCost);

            // if (!Config::get().keepCargoModifyPickup)

            for (const auto& car : train.cars) 
            {   
                auto* vehObj = ObjectManager::get<VehicleObject>(car.body->objectId);
                if (!Config::get().keepCargoModifyPickup) 
                {
                  for (auto& component : car)
                  {
                      removeAllCargo(component);
                  }
                }
                if (vehObj->power != 0) // check that it's a power car. 
                  {
                    // Check for obsolescence.
                    auto year = getCurrentYear();
                    if (vehObj->designed > year || vehObj->obsolete < year) {
                        return kFailure;
                    }

                    // Todo: Add extra cheat that prevents removal
                    // of cargo for power units.
                    for (auto& component : car)
                    {
                        
                        removeAllCargo(component);
                        // Max reliability
                        // implicit cast; vehObj->reliability is 8-bits.
                        int32_t maxRely = 256 * vehObj->reliability;
                        if (vehObj->designed + 2 > getCurrentYear())
                        {
                            // Vanilla intended to reduce reliability by 1/8th twice for the first two years after 
                            // the year designed (i.e. the reliability is lower FOR the first two years,
                            // not that the reliability BECOMES lower each of the first two years),
                            // then reduce reliability by 1/8th once for the third year. However, 
                            // a bug meant that the two 1/8th reductions were always applied.
                            maxRely -= maxRely / 8;
                            maxRely -= maxRely / 8;
                        }
                        if (maxRely != 0)
                        {
                            maxRely += 255;
                        }
                        
                        // Actually apply the reliability.

                    }

                    
                  }                

            }

            /*
            if (!vehObj->hasFlags(VehicleObjectFlags::refittable))
            {
                setErrorText(StringIds::empty);
                return 0;
            }
            */

            // Don't know what this was for.
            /*
            if (!hasFlags(flags, Flags::apply))
            {
                return 0;
            }
            */

            // uint16_t maxPrimaryCargo = vehObj->maxCargo[0];
            // auto cargoTypes = vehObj->compatibleCargoCategories[0];
            // auto primaryCargoId = Numerics::bitScanForward(cargoTypes);
            // uint16_t maxCargoUnits = Vehicles::getNumUnitsForCargo(maxPrimaryCargo, primaryCargoId, args.cargoType);

            /*
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

                if (cargoObject->cargoCategory == primaryCargoObj->cargoCategory)
                {
                    acceptedTypes |= 1 << cargoId;
                }
            }
            car.body->primaryCargo.acceptedTypes = acceptedTypes;
            */
            // head->updateTrainProperties();
            // Ui::WindowManager::invalidate(Ui::WindowType::vehicle, static_cast<Ui::WindowNumber_t>(head->id));

            return netCost;
        }
        catch (std::runtime_error&)
        {
            return kFailure;
        }
    }

    void vehicleRefreshLoco(registers& regs, const Flags flags)
    {
        regs.ebx = vehicleRefreshLoco(VehicleRefreshLocoArgs(regs), flags);
    }
}
