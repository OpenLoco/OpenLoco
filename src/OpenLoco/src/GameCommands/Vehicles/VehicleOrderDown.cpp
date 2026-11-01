#include "GameCommands/Vehicles/VehicleOrderDown.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"

namespace OpenLoco::GameCommands
{
    // 0x00470E06
    static uint32_t vehicleOrderDown(const VehicleOrderDownArgs& args, Flags flags)
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

        // Figure out which orders to swap
        Vehicles::OrderRingView orders(head->orderTableOffset, args.orderOffset);
        auto iterator = orders.begin();
        auto& currentOrder = *(iterator);
        auto& nextOrder = *(++iterator);

        // Don't swap if we've looped around
        if (nextOrder.getOffset() < currentOrder.getOffset())
        {
            return 0;
        }

        const auto currentOrderOffset = currentOrder.getOffset() - head->orderTableOffset;
        const auto nextOrderOffset = nextOrder.getOffset() - head->orderTableOffset;
        const auto oldOffsetDiff = nextOrder.getOffset() - currentOrder.getOffset();

        const auto newOffsetDiff = Vehicles::OrderManager::swapAdjacentOrders(currentOrder, nextOrder);

        for (auto* orderHead : VehicleManager::VehicleList())
        {
            if (orderHead->orderTableOffset != head->orderTableOffset)
            {
                continue;
            }

            const bool currentOrderIsActive = orderHead->currentOrder == currentOrderOffset;
            const bool nextOrderIsActive = orderHead->currentOrder == nextOrderOffset;

            if (currentOrderIsActive)
            {
                orderHead->currentOrder += newOffsetDiff;
            }
            else if (nextOrderIsActive)
            {
                orderHead->currentOrder -= oldOffsetDiff;
            }
        }

        return 0;
    }

    void vehicleOrderDown(registers& regs, const Flags flags)
    {
        regs.ebx = vehicleOrderDown(VehicleOrderDownArgs(regs), flags);
    }
}
