#include "ScenarioManager.h"
#include "Interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::scenariomgr
{
    // 0x0044452F
    void loadIndex(uint8_t al)
    {
        registers regs;
        regs.al = al;
        call(0x0044452F, regs);
    }
}
