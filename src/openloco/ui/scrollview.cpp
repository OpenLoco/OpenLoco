#include "scrollview.h"
#include "../interop/interop.hpp"
#include "../ui.h"
#include "WindowManager.h"

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
        regs.esi = (loco_ptr)window;
        regs.edi = (loco_ptr)widget;

        call(0x004C8EF0, regs);

        *output_x = regs.ax;
        *output_y = regs.bx;
        *output_scroll_area = (scroll_part)regs.cx;
        *scroll_id = regs.edx;
    }

    // 0x004CA1ED
    void update_thumbs(window* window, widget_index widgetIndex)
    {
        registers regs;

        regs.esi = (loco_ptr)window;
        regs.ebx = window->get_scroll_data_index(widgetIndex) * sizeof(scroll_area_t);
        regs.edi = (loco_ptr)&((ui::widget_t*)(uintptr_t )window->widgets)[widgetIndex];
        call(0x4CA1ED, regs);
    }
}
