#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"
#include "../window.h"

using namespace openloco::interop;

namespace openloco::ui::windows::ObjectSelectionWindow
{
    // 0x00472A20
    window* open()
    {
        registers regs;
        call(0x00472A20, regs);

        return reinterpret_cast<window*>(regs.esi);
    }
}
