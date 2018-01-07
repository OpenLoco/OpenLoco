#include "interop/interop.hpp"
#include "progressbar.h"

namespace openloco::progressbar
{
    // 0x004CF5C5
    // eax: maximum
    void begin(int32_t maximum, int32_t edx)
    {
        registers regs;
        regs.eax = maximum;
        regs.edx = edx;
        LOCO_CALLPROC_X(0x004CF5C5, regs);
    }

    // 0x004CF621
    // eax: value
    void increment(int32_t value)
    {
        registers regs;
        regs.eax = value;
        LOCO_CALLPROC_X(0x004CF621, regs);
    }

    // 0x004CF60B
    void end()
    {
        LOCO_CALLPROC_X(0x004CF60B);
    }
}
