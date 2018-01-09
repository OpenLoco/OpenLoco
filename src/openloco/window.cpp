#include "interop/interop.hpp"
#include "window.h"

using namespace openloco::ui;

void window::sub_4CA17F()
{
    registers regs;
    regs.esi = (int32_t)this;
    LOCO_CALLPROC_X(0x004CA17F, regs);
}
