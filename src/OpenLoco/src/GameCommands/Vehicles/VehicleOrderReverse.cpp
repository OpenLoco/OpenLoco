#include "GameCommands/Vehicles/VehicleOrderReverse.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleHead.h"

namespace OpenLoco::GameCommands
{
    static uint32_t vehicleOrderReverse(const VehicleOrderReverseArgs& args, uint8_t flags)
    {
        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return GameCommands::kFailure;
        }

        GameCommands::setPosition(head->position);
        if (!(flags & GameCommands::Flags::apply))
        {
            return 0;
        }

        Ui::WindowManager::invalidateOrderPageByVehicleNumber(enumValue(head->id));

        head->currentOrder = Vehicles::OrderManager::reverseVehicleOrderTable(head->orderTableOffset, head->currentOrder);

        return 0;
    }

    void vehicleOrderReverse(registers& regs, const uint8_t flags)
    {
        regs.ebx = vehicleOrderReverse(VehicleOrderReverseArgs(regs), flags);
    }
}
