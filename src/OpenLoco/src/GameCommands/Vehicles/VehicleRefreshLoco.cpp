#include "GameCommands/Vehicles/VehicleRefreshLoco.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle1.h"
#include "Vehicles/VehicleBody.h"
#include "Vehicles/VehicleBogie.h"
#include "Vehicles/VehicleHead.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    
    static uint32_t vehicleRefreshLoco(const VehicleRefreshLocoArgs& args, const Flags flags)
    {
        // moved to end
        //setExpenditureType(ExpenditureType::VehiclePurchases);

        EntityId headId = args.head;

        try
        {
            Vehicles::Vehicle train(headId);
            auto& head = train.head;

            // Fail if the vehicle can't be modified.
            if (!head->canBeModified())
            {
                return kFailure;
            }

            if (!checkCompanyCompatibility(head->owner))
            {
                return kFailure;
            }
                        
            currency32_t refundCost = 0;
            currency32_t purchaseCost = 0;

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
            }

            // Note: Refund cost might already be negative signed;
            // make sure netCost <= purchaseCost.
            currency32_t netCost = purchaseCost - refundCost;
            assert (netCost <= purchaseCost); // safety check

            uint32_t thisDay = getCurrentDay();
            // maybe replace each call to getCurrentYear() with
            // a variable like this?

            // Actually replace the vehicles if apply is on.
            if (hasFlags(flags, Flags::apply))
            {
                for (const auto& car : train.cars)
                {
                    auto* vehObj = ObjectManager::get<VehicleObject>(car.body->objectId);
                    // Remove cargo from all components
                    // (unless keepCargoModifyPickup is on)
                    // Possibly todo: add check that the user knows that
                    // cargo will be removed.
                    if (!Config::get().keepCargoModifyPickup)
                    {
                        for (auto& component : car)
                        {
                            removeAllCargo(component);
                        }
                    }
                    if (vehObj->power != 0) // check that it's a power car.
                    {
                        // Check for obsolescence, unless build locked vehicles is on.
                        if (!Config::get().buildLockedVehicles)
                        {
                            auto year = getCurrentYear();
                            if (vehObj->designed > year || vehObj->obsolete < year)
                            {
                                // StringIds::vehicle_is_locked
                                return kFailure;
                            }
                        }

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
                        // Only the front bogie stores the reliability.
                        car.front->reliability = maxRely;
                        // Reset the breakdown timer.
                        car.front->timeoutToBreakdown = 0xFFFF;
                        // Reset the refund cost to 7/8 * (new cost).
                        auto temp_cost = Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
                        temp_cost -= temp_cost / 8;
                        car.front->refundCost = temp_cost;

                        // Todo: Add extra cheat that prevents removal
                        // of cargo from power units.
                        //
                        // This checks whether "keepCargoModifyPickup"
                        // is *ON*, since if it is off, then the car
                        // would have already had its cargo removed
                        // earlier. This prevents "removeAllCargo" from
                        // being run twice.
                        if (Config::get().keepCargoModifyPickup)
                        {
                            for (auto& component : car)
                            {

                                removeAllCargo(component);
                            }
                        }
                    }

                    // Set the age of the vehicle as a whole to 0.
                    // (i.e. make today the day of creation)
                    train.veh1->dayCreated = thisDay;
                }
            }

            setExpenditureType(ExpenditureType::VehiclePurchases);
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
        // In case you want to inspect the value before returning it:
        // uint32_t rslt = vehicleRefreshLoco(VehicleRefreshLocoArgs(regs), flags);
        // regs.ebx = rslt;
    }
}
