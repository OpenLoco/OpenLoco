#include "scenariomgr.h"
#include "interop/interop.hpp"

using namespace openloco;
using namespace openloco::interop;

scenariomanager openloco::g_scenariomgr;

// 0x0044452F
void scenariomanager::load_index(uint8_t al)
{
    registers regs;
    regs.al = al;
    call(0x0044452F, regs);
}
