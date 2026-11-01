#include "GameCommands/Vehicles/VehicleOrderUp.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"

namespace OpenLoco::GameCommands
{
    // 0x00470CD2
    static uint32_t vehicleOrderUp(const VehicleOrderUpArgs& args, Flags flags)
    {
        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return GameCommands::kFailure;
        }

        GameCommands::setPosition(head->position);
        if (!hasFlags(flags, GameCommands::Flags::apply))
        {
            return 0;
        }

        if (args.orderOffset > head->sizeOfOrderTable)
        {
            return kFailure;
        }
        Vehicles::OrderManager::invalidateOrderPagesByOrderTableOffset(head->orderTableOffset);

        // Can't move up if the order is already at the start of the table
        if (args.orderOffset == 0)
        {
            return 0;
        }

        // Figure out which orders to swap
        Vehicles::OrderRingView orders(head->orderTableOffset, args.orderOffset);
        Vehicles::Order* prevOrder = nullptr;
        auto iterator = orders.begin();

        // Continue looping until we've found the target order
        do
        {
            prevOrder = &*iterator;
            iterator++;
        } while ((*iterator).getOffset() - head->orderTableOffset != args.orderOffset);

        auto& currentOrder = *(iterator);

        const auto prevOrderOffset = prevOrder->getOffset() - head->orderTableOffset;
        const auto currentOrderOffset = currentOrder.getOffset() - head->orderTableOffset;
        const auto oldOffsetDiff = currentOrder.getOffset() - prevOrder->getOffset();

        const auto newOffsetDiff = Vehicles::OrderManager::swapAdjacentOrders(*prevOrder, currentOrder);

        for (auto* orderHead : VehicleManager::VehicleList())
        {
            if (orderHead->orderTableOffset != head->orderTableOffset)
            {
                continue;
            }

            const bool prevOrderIsActive = orderHead->currentOrder == prevOrderOffset;
            const bool currentOrderIsActive = orderHead->currentOrder == currentOrderOffset;

            if (prevOrderIsActive)
            {
                orderHead->currentOrder += newOffsetDiff;
            }
            else if (currentOrderIsActive)
            {
                orderHead->currentOrder -= oldOffsetDiff;
            }
        }

        return 0;
    }

    void vehicleOrderUp(registers& regs, const Flags flags)
    {
        regs.ebx = vehicleOrderUp(VehicleOrderUpArgs(regs), flags);
    }
}
