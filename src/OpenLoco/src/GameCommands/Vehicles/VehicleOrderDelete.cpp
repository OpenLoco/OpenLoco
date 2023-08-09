#include "VehicleOrderDelete.h"
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

        Vehicles::OrderManager::deleteOrder(head, args.orderOffset);

        return 0;
    }

    void vehicleOrderDelete(registers& regs)
    {
        regs.ebx = vehicleOrderDelete(VehicleOrderDeleteArgs(regs), regs.bl);
    }
}
