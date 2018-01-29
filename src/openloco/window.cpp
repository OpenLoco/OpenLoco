#include "window.h"
#include "interop/interop.hpp"

using namespace openloco;
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

bool window::call_tooltip(int16_t widget_index)
{
    registers regs;
    regs.ax = widget_index;
    regs.esi = (int32_t)this;
    call((uint32_t)this->event_handlers->event_19, regs);

    return regs.ax != (int16_t)string_ids::null;
}

void window::call_prepare_draw()
{
    if (this->event_handlers->prepare_draw == nullptr)
    {
        return;
    }

    if ((uint32_t)(this->event_handlers->prepare_draw) < 0x4d7000)
    {
        registers regs;
        regs.esi = (uint32_t)this;
        call((uint32_t)this->event_handlers->prepare_draw, regs);
        return;
    }

    this->event_handlers->prepare_draw(this);
}

void window::draw(gfx::drawpixelinfo_t* dpi)
{
    if (this->event_handlers->draw == nullptr)
    {
        return;
    }

    if ((uint32_t)(this->event_handlers->draw) < 0x4d7000)
    {
        registers regs;
        regs.esi = (uint32_t)this;
        regs.edi = (uint32_t)dpi;
        call((uint32_t)this->event_handlers->draw, regs);
        return;
    }

    this->event_handlers->draw(this, dpi);
}
