#include "interop/interop.hpp"
#include "scenariomgr.h"

namespace openloco::scenariomgr
{
    // 0x0044452F
    void load_index(uint8_t al)
    {
        registers regs;
        regs.al = al;
        LOCO_CALLPROC_X(0x0044452F, regs);
    }
}
