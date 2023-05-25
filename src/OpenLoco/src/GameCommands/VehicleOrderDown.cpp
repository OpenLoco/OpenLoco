#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00470E06
    static uint32_t vehicleOrderDown(const VehicleOrderDownArgs& args, uint8_t flags)
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

        // Before we proceed, we keep track of the currently active order
        bool currentOrderIsActive = head->currentOrder == currentOrder.getOffset() - head->orderTableOffset;
        bool nextOrderIsActive = head->currentOrder == nextOrder.getOffset() - head->orderTableOffset;

        printf("currentOrderIsActive: %d\n", currentOrderIsActive);
        printf("nextOrderIsActive: %d\n", nextOrderIsActive);

        // Actually swap the two orders
        const auto offsetDiff = Vehicles::swapAdjacentOrders(currentOrder, nextOrder);

        // Compensate if we swapped the current order around
        if (currentOrderIsActive)
        {
            head->currentOrder += offsetDiff;
        }
        else if (nextOrderIsActive)
        {
            head->currentOrder -= offsetDiff;
        }

        return 0;
    }

    void vehicleOrderDown(registers& regs)
    {
        regs.ebx = vehicleOrderDown(VehicleOrderDownArgs(regs), regs.bl);
    }
}
