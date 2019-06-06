#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town_list
{
    // 0x00499C83
    window* open()
    {
        registers regs;
        call(0x00499C83, regs);
        return (window*)(uintptr_t)regs.esi;
    }
}
