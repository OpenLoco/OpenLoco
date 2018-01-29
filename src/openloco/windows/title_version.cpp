#include "../graphics/colours.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../window.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static widget widgets[] = {
        { 0x1E, 0, 0, 0, 0, 0, { 0 }, 0 }
    };

    static ui::window_event_list_t events;

    static void draw(window* window);

    uint8_t* buffer;
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* open_title_version()
    {
        auto window = openloco::ui::windowmgr::create_window(
            window_type::openloco_version,
            8,
            ui::height() - 11 - 8,
            512,
            30,
            (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6),
            (void*)&events);
        window->widgets = widgets;

        for (int i = 0; i < 26; i++)
        {
            events.events[i] = 0x0042A035;
        }
        events.prepare_draw = (void (*)(ui::window*))0x0042A035;
        events.draw = draw;
        events.event_28 = 0x0042A035;

        return window;
    }

    // 0x00439236
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        char str[128] = "OpenLoco 2018.1 (0f2d6ec)";

        {
            registers regs;
            regs.cx = window->x;
            regs.dx = window->y;
            regs.bx = 160;
            regs.al = colour::white | 0x20;
            regs.edi = (uint32_t)dpi;
            regs.esi = (uint32_t)str;
            call(0x451025, regs);
        }
    }
}
