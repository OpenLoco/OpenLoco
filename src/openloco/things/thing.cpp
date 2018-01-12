#include "../interop/interop.hpp"
#include "thing.h"

using namespace openloco;

// 0x0046FC83
void thing::move_to(loc16 loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = (int32_t)this;
    LOCO_CALLPROC_X(0x0046FC83, regs);
}
