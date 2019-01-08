#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::industry_list
{
    // 0x004577FF
    window* open()
    {
        registers regs;
        call(0x004577FF, regs);
        return (window*)(uintptr_t )regs.esi;
    }
}
