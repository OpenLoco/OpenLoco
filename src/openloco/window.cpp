#include "window.h"
#include "interop/interop.hpp"

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::ui;

template<typename T>
static bool is_interop_event(T e)
{
    return (uint32_t)e < 0x004D7000;
}

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

bool window::call_tooltip(int16_t widget_index)
{
    registers regs;
    regs.ax = widget_index;
    regs.esi = (int32_t)this;
    call((int32_t)this->event_handlers->event_19, regs);
    return regs.ax != (int16_t)string_ids::null;
}

void window::call_prepare_draw()
{
    if (event_handlers->prepare_draw != nullptr)
    {
        if (is_interop_event(event_handlers->prepare_draw))
        {
            registers regs;
            regs.esi = (int32_t)this;
            call((int32_t)this->event_handlers->prepare_draw, regs);
        }
        else
        {
            event_handlers->prepare_draw(this);
        }
    }
}
