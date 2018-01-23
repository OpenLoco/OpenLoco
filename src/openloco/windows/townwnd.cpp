#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    // 0x00498E9B
    void sub_498E9B(window* w)
    {
        w->enabled_widgets |= 2;
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
    window* open_town_window(uint16_t townId)
    {
        registers regs;
        regs.dx = townId;
        call(0x00499B7E, regs);
        return (window*)regs.esi;
    }
}
