#include "window.h"
#include "interop/interop.hpp"

using namespace openloco::interop;
using namespace openloco::ui;

// 0x004CA4BD
void window::invalidate()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004CA4BD, regs);
}

void window::sub_4CA17F()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004CA17F, regs);
}
