#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::vehicle_list
{
    // 0x004C19DC
    window* open(uint16_t companyId, uint8_t type)
    {
        window* vehicle_list = WindowManager::bringToFront(WindowType::vehicleList, companyId);
        if (vehicle_list == nullptr)
        {
            registers regs;
            regs.eax = (uint32_t)companyId;
            regs.ebx = type;
            call(0x004C1A05, regs);
            vehicle_list = (window*)regs.esi;
        }
        else
            vehicle_list->call_on_mouse_up(type + 4);

        return vehicle_list;
    }
}
