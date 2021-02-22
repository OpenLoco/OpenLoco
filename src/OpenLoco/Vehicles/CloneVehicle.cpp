#include "../GameCommands.h"
#include "../Interop/Interop.hpp"
#include "../Things/ThingManager.h"
#include "../Ui/WindowManager.h"
#include "Orders.h"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static uint32_t cloneVehicle(uint16_t head, uint8_t flags)
    {
        static loco_global<uint16_t, 0x0113642A> _113642A;
        Vehicles::Vehicle existingTrain(head);
        Vehicles::VehicleHead* newHead = nullptr;

        // Get total cost for a new vehicle
        if (!(flags & GameCommands::apply))
        {
            uint32_t totalCost = 0;
            for (auto& car : existingTrain.cars)
            {
                auto cost = GameCommands::queryDo_5(car.front->object_id, -1);
                if (cost == GameCommands::FAILURE)
                {
                    totalCost = GameCommands::FAILURE;
                    break;
                }
                else
                {
                    totalCost += cost;
                }
            }

            if (totalCost == GameCommands::FAILURE)
            {
                return GameCommands::FAILURE;
            }
            return totalCost;
        }

        uint32_t totalCost = 0;
        for (auto& car : existingTrain.cars)
        {
            uint32_t cost = 0;
            if (newHead == nullptr)
            {
                cost = GameCommands::do_5(car.front->object_id, -1);
                auto* newVeh = ThingManager::get<Vehicles::VehicleBase>(_113642A);
                if (newVeh == nullptr)
                {
                    return GameCommands::FAILURE;
                }
                newHead = ThingManager::get<Vehicles::VehicleHead>(newVeh->getHead());
            }
            else
            {
                cost = GameCommands::do_5(car.front->object_id, newHead->head);
            }
            if (cost == GameCommands::FAILURE)
            {
                totalCost = GameCommands::FAILURE;
                break;
            }
            else
            {
                totalCost += cost;
            }
        }
        if (totalCost == GameCommands::FAILURE || newHead == nullptr)
        {
            return GameCommands::FAILURE;
        }

        // Copy orders
        std::vector<std::shared_ptr<Vehicles::Order>> clonedOrders;
        for (auto& existingOrders : Vehicles::OrderRingView(existingTrain.head->orderTableOffset))
        {
            clonedOrders.push_back(existingOrders.clone());
        }

        for (auto& order : clonedOrders)
        {
            // Do not cache this as it will be a different value every iteration
            auto chosenOffset = newHead->sizeOfOrderTable - 1;
            GameCommands::do_35(newHead->id, order->getRaw(), chosenOffset);
        }

        // Copy express/local
        if (existingTrain.veh1->var_48 & (1 << 1))
        {
            GameCommands::do12(newHead->id, 2);
        }
        return totalCost;
    }

    void cloneVehicle(registers& regs)
    {
        regs.ebx = cloneVehicle(regs.ax, regs.bl);
    }
}
