#include "interop/interop.hpp"
#include "scenariomgr.h"

using namespace openloco::interop;

namespace openloco::scenariomgr
{
    // 0x0044452F
    void load_index(uint8_t al)
    {
        registers regs;
        regs.al = al;
        //call(0x0044452F, regs);
    }
}
