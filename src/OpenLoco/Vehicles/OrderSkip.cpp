#include "../GameCommands.h"
#include "../Interop/Interop.hpp"
#include "../Things/ThingManager.h"
#include "../Ui/WindowManager.h"
#include "Orders.h"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    loco_global<uint16_t, 0x009C68E0> gameCommandMapX;
    loco_global<uint16_t, 0x009C68E2> gameCommandMapY;
    loco_global<uint16_t, 0x009C68E4> gameCommandMapZ;

    // 0x0047071A
    static uint32_t orderSkip(uint16_t headId, uint8_t flags)
    {
        auto* head = ThingManager::get<Vehicles::VehicleHead>(headId);
        if (head == nullptr)
        {
            return GameCommands::FAILURE;
        }

        gameCommandMapX = head->x;
        gameCommandMapY = head->y;
        gameCommandMapZ = head->z;
        if (!(flags & GameCommands::apply))
        {
            return 0;
        }

        Ui::WindowManager::sub_4B93A5(head->id);

        OrderRingView orders(head->orderTableOffset, head->currentOrder);
        auto nextOrder = ++orders.begin();
        head->currentOrder = nextOrder->getOffset() - head->orderTableOffset;
        return 0;
    }

    void orderSkip(registers& regs)
    {
        regs.ebx = orderSkip(regs.di, regs.bl);
    }
}
