#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Interop/Interop.hpp"
#include "Ui/WindowManager.h"
#include "Orders.h"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x0047071A
    static uint32_t orderSkip(EntityId headId, uint8_t flags)
    {
        auto* head = EntityManager::get<Vehicles::VehicleHead>(headId);
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

        OrderRingView orders(head->orderTableOffset, head->currentOrder);
        auto nextOrder = ++orders.begin();
        head->currentOrder = nextOrder->getOffset() - head->orderTableOffset;
        return 0;
    }

    void orderSkip(registers& regs)
    {
        regs.ebx = orderSkip(EntityId(regs.di), regs.bl);
    }
}
