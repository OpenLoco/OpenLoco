#include "interop/interop.hpp"
#include "window.h"

using namespace openloco::interop;
using namespace openloco::ui;

void window::sub_4CA17F()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004CA17F, regs);
}
