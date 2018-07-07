#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::windows::TownWindow
{
    // 0x00498E9B
    void sub_498E9B(Window* w)
    {
        w->enableWidgets(1);
#ifdef _DISABLE_TOWN_RENAME_
        if (is_editor_mode())
        {
            w->enabled_widgets &= ~2;
        }
#endif
    }

    // 0x00499B7E
    // dx: townId
    // esi: {return}
    Window* open(town_id_t townId)
    {
        registers regs;
        regs.dx = townId;
        call(0x00499B7E, regs);
        return (Window*)regs.esi;
    }
}
