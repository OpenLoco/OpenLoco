#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0047071A
    static uint32_t vehicleOrderSkip(EntityId headId, uint8_t flags)
    {
        auto* head = EntityManager::get<Vehicles::VehicleHead>(headId);
        if (head == nullptr)
        {
            return FAILURE;
        }

        setPosition(head->position);
        if (!(flags & Flags::apply))
        {
            return 0;
        }

        Ui::WindowManager::sub_4B93A5(enumValue(head->id));

        Vehicles::OrderRingView orders(head->orderTableOffset, head->currentOrder);
        auto nextOrder = ++orders.begin();
        head->currentOrder = nextOrder->getOffset() - head->orderTableOffset;
        return 0;
    }

    void vehicleOrderSkip(registers& regs)
    {
        regs.ebx = vehicleOrderSkip(EntityId(regs.di), regs.bl);
    }
}
