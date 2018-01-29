#include "../graphics/colours.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static void paint(ui::window* window, gfx::drawpixelinfo_t* dpi);

    static loco_global<window_event_list[1], 0x004F9FB0> _events;

    ui::window* open_title_logo()
    {
        _events[0].draw = paint;

        auto window = openloco::ui::windowmgr::create_window(
            window_type::title_logo,
            0,
            0,
            298,
            170,
            1 << 1,
            (window_event_list*)&_events[0]);
        window->widgets = (ui::widget_t*)0x00509e6c;
        window->enabled_widgets = (1 << 0);

        window->init_scroll_widgets();

        window->flags |= 0x10;
        window->colours[0] = colour::translucent(colour::grey);
        window->colours[1] = colour::translucent(colour::grey);

        return window;
    }

    // 0x00439298
    static void paint(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        gfx::draw_image(dpi, window->x, window->y, 3624);
    }

    // 0x004392AD
    static void on_mouse_up(window* window, uint16_t widget)
    {
        switch (widget)
        {
            case 0:
                about::open();
                break;
        }
    }
}
