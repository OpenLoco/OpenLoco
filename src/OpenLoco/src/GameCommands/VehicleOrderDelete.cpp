#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0047057A
    static uint32_t vehicleOrderDelete(const VehicleOrderDeleteArgs& args, uint8_t flags)
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

        // Find out what type the selected order is
        Vehicles::OrderRingView orders(head->orderTableOffset, args.orderOffset);
        auto& selectedOrder = *(orders.begin());

        // Bookkeeping: change order table sizes
        // TODO: this should probably be done after shifting orders? Following original sub for now
        auto removeOrderSize = selectedOrder.getSize();
        head->sizeOfOrderTable -= removeOrderSize;
        Vehicles::OrderManager::numOrders() -= removeOrderSize;

        // Move orders in the order table, effectively removing the order
        Vehicles::OrderManager::shiftOrdersDown(head->orderTableOffset + args.orderOffset, removeOrderSize);

        // Compensate other vehicles to use new table offsets
        Vehicles::OrderManager::reoffsetVehicleOrderTables(head->orderTableOffset + args.orderOffset, -removeOrderSize);

        return 0;
    }

    void vehicleOrderDelete(registers& regs)
    {
        regs.ebx = vehicleOrderDelete(VehicleOrderDeleteArgs(regs), regs.bl);
    }
}
