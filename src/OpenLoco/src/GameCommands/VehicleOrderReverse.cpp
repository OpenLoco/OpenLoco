#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static uint32_t vehicleOrderReverse(const VehicleOrderReverseArgs& args, uint8_t flags)
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

        head->currentOrder = Vehicles::reverseVehicleOrderTable(head->orderTableOffset, head->currentOrder);

        return 0;
    }

    void vehicleOrderReverse(registers& regs)
    {
        regs.ebx = vehicleOrderReverse(VehicleOrderReverseArgs(regs), regs.bl);
    }
}
