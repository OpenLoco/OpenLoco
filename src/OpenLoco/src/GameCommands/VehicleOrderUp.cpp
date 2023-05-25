#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00470CD2
    static uint32_t vehicleOrderUp(const VehicleOrderUpArgs& args, uint8_t flags)
    {
        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return GameCommands::FAILURE;
        }

        GameCommands::setPosition(head->position);
        if (!(flags & GameCommands::Flags::apply))
        {
            return 0;
        }

        Ui::WindowManager::sub_4B93A5(enumValue(head->id));

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
        }
        while ((*iterator).getOffset() - head->orderTableOffset != args.orderOffset);

        auto& currentOrder = *(iterator);

        // Before we proceed, we keep track of the currently active order
        bool prevOrderIsActive = head->currentOrder == prevOrder->getOffset() - head->orderTableOffset;
        bool currentOrderIsActive = head->currentOrder == currentOrder.getOffset() - head->orderTableOffset;
        auto oldOffsetDiff = currentOrder.getOffset() - prevOrder->getOffset();

        // Actually swap the two orders
        const auto newOffsetDiff = Vehicles::swapAdjacentOrders(*prevOrder, currentOrder);

        // Compensate if we swapped the current order around
        if (prevOrderIsActive)
        {
            head->currentOrder += newOffsetDiff;
        }
        else if (currentOrderIsActive)
        {
            head->currentOrder -= oldOffsetDiff;
        }

        return 0;
    }

    void vehicleOrderUp(registers& regs)
    {
        regs.ebx = vehicleOrderUp(VehicleOrderUpArgs(regs), regs.bl);
    }
}
