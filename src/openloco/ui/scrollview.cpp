#include "scrollview.h"
#include "../interop/interop.hpp"
#include "../ui.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::scrollview
{

    // 0x004C8EF0
    void get_part(
        ui::window* window,
        ui::widget_t* widget,
        int16_t x,
        int16_t y,
        int16_t* output_x,
        int16_t* output_y,
        scroll_part* output_scroll_area,
        int32_t* scroll_id)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        regs.esi = (uint32_t)window;
        regs.edi = (uint32_t)widget;

        call(0x004C8EF0, regs);

        *output_x = regs.ax;
        *output_y = regs.bx;
        *output_scroll_area = (scroll_part)regs.cx;
        *scroll_id = regs.edx;
    }
}
