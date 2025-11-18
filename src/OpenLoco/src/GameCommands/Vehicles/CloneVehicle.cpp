#include "CloneVehicle.h"
#include "CreateVehicle.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "VehicleChangeRunningMode.h"
#include "VehicleOrderInsert.h"
#include "VehicleRefit.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static void copyVehicleColours(Vehicles::VehicleBase* source, Vehicles::VehicleBase* target)
    {
        auto* sourceHead = source;
        auto* targetHead = target;
        while (sourceHead != nullptr && targetHead != nullptr)
        {
            targetHead->setColourScheme(sourceHead->getColourScheme());
            sourceHead = sourceHead->nextVehicleComponent();
            targetHead = targetHead->nextVehicleComponent();
        }
    }

    static uint32_t cloneVehicle(EntityId head, uint8_t flags)
    {
        Vehicles::Vehicle existingTrain(head);
        Vehicles::VehicleHead* newHead = nullptr;

        // Get total cost for a new vehicle
        if (!(flags & Flags::apply))
        {
            uint32_t totalCost = 0;
            for (auto& car : existingTrain.cars)
            {
                VehicleCreateArgs args{};
                args.vehicleId = EntityId::null;
                args.vehicleType = car.front->objectId;

                const auto cost = doCommand(args, 0);
                if (cost == FAILURE)
                {
                    totalCost = FAILURE;
                    break;
                }
                else
                {
                    totalCost += cost;
                }
            }

            if (totalCost == FAILURE)
            {
                return FAILURE;
            }
            return totalCost;
        }

        uint16_t cargoType = 0;
        uint32_t totalCost = 0;
        for (auto& car : existingTrain.cars)
        {
            uint32_t cost = 0;
            if (newHead == nullptr)
            {
                VehicleCreateArgs args{};
                args.vehicleId = EntityId::null;
                args.vehicleType = car.front->objectId;

                cost = doCommand(args, Flags::apply);
                cargoType = car.body->primaryCargo.type;

                auto* newVeh = EntityManager::get<Vehicles::VehicleBase>(getLegacyReturnState().lastCreatedVehicleId);
                if (newVeh == nullptr)
                {
                    return FAILURE;
                }
                newHead = EntityManager::get<Vehicles::VehicleHead>(newVeh->getHead());
            }
            else
            {
                VehicleCreateArgs args{};
                args.vehicleId = newHead->head;
                args.vehicleType = car.front->objectId;

                cost = doCommand(args, Flags::apply);
            }
            if (cost == FAILURE)
            {
                totalCost = FAILURE;
                break;
            }
            else
            {
                totalCost += cost;
            }
        }
        if (totalCost == FAILURE || newHead == nullptr)
        {
            return FAILURE;
        }

        copyVehicleColours(existingTrain.head, newHead);

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

            VehicleOrderInsertArgs args{};
            args.head = newHead->id;
            args.orderOffset = chosenOffset;
            args.rawOrder = order->getRaw();
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        // Copy express/local
        if ((existingTrain.veh1->var_48 & Vehicles::Flags48::expressMode) != Vehicles::Flags48::none)
        {
            GameCommands::VehicleChangeRunningModeArgs args{};
            args.head = newHead->id;
            args.mode = GameCommands::VehicleChangeRunningModeArgs::Mode::toggleLocalExpress;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        // Copy cargo refit status (only applies to boats and airplanes)
        if (newHead->vehicleType == VehicleType::ship || newHead->vehicleType == VehicleType::aircraft)
        {
            VehicleRefitArgs args{};
            args.head = newHead->head;
            args.cargoType = cargoType;
            doCommand(args, Flags::apply);
        }

        return totalCost;
    }

    void cloneVehicle(registers& regs)
    {
        regs.ebx = cloneVehicle(EntityId(regs.ax), regs.bl);
    }
}
