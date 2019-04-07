#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::station_list
{
    window* open(uint16_t playerId, uint8_t type)
    {
        registers regs;
        regs.eax = (uint32_t)playerId;
        call(0x00490F6C, regs);

        auto station_list = (window*)regs.esi;
        station_list->call_on_mouse_up(type + 4);

        return station_list;
    }
}
