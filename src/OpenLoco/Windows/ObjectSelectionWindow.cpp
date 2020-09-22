#include "../Interop/Interop.hpp"
#include "../Ui/WindowManager.h"
#include "../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::ui::windows::ObjectSelectionWindow
{
    // 0x00472A20
    window* open()
    {
        registers regs;
        call(0x00472A20, regs);

        return reinterpret_cast<window*>(regs.esi);
    }
}
