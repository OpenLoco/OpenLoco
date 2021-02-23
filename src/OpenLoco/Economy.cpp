#include "Economy.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Economy
{
    // 0x0046E239
    void sub_46E239()
    {
        call(0x0046E239);
    }

    // 0x0046E2C0
    void sub_46E2C0(uint16_t year)
    {
        registers regs;
        regs.ax = year;
        call(0x0046E2C0, regs);
    }
}
