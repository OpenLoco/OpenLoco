#include "gfx.h"

namespace openloco::gfx
{
    loco_global<drawpixelinfo_t, 0x0050B884> screen_dpi;

    // 0x00447485
    // edi: dpi
    // ebp: fill
    void clear(drawpixelinfo_t &dpi, uint32_t fill)
    {
        registers regs;
        regs.edi = (int32_t)&dpi;
        regs.ebp = (int32_t)fill;
        LOCO_CALLPROC_X(0x00447485, regs);
    }
}
