#include "thing.h"
#include "../interop/interop.hpp"

using namespace openloco;
using namespace openloco::interop;

// 0x0046FC83
void thing::move_to(loc16 loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = (int32_t)this;
    call(0x0046FC83, regs);
}
