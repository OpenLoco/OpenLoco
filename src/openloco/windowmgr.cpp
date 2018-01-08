#include "interop/interop.hpp"
#include "windowmgr.h"

namespace openloco::ui::windowmgr
{
    // 0x004392BD
    void resize()
    {
        LOCO_CALLPROC_X(0x004392BD);
    }

    // 0x004C9B56
    window * find(window_type type)
    {
        constexpr uint16_t FLAG_BY_TYPE = 1 << 7;

        registers regs;
        regs.cx = (uint8_t)type | FLAG_BY_TYPE;
        LOCO_CALLFUNC_X(0x004C9B56, regs);
        return (window *)regs.esi;
    }

    // 0x004C9B56
    window * find(window_type type, uint16_t index)
    {
        registers regs;
        regs.cl = (uint8_t)type & 0xFF;
        regs.dx = index;
        LOCO_CALLFUNC_X(0x004C9B56, regs);
        return (window *)regs.esi;
    }
}

