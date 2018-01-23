#include "progressbar.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::progressbar
{
    // 0x004CF5C5
    // eax: maximum
    void begin(string_id stringId, int32_t edx)
    {
        registers regs;
        regs.eax = stringId;
        regs.edx = edx;
        call(0x004CF5C5, regs);
    }

    // 0x004CF621
    // eax: value
    void set_progress(int32_t value)
    {
        registers regs;
        regs.eax = value;
        call(0x004CF621, regs);
    }

    // 0x004CF60B
    void end()
    {
        call(0x004CF60B);
    }
}
