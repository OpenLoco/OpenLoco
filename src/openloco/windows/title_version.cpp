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

    static ui::window_event_list _events;

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* open_title_version()
    {
        auto width = 512;
        auto height = 16;
        auto window = openloco::ui::windowmgr::create_window(
            window_type::openloco_version,
            8,
            ui::height() - height,
            width,
            height,
            (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6),
            (void*)&_events);
        window->widgets = widgets;

        _events.prepare_draw = (void (*)(ui::window*))0x0042A035;
        _events.draw = draw;
        _events.event_28 = 0x0042A035;

        return window;
    }

    // 0x00439236
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto versionInfo = get_version_info();
        {
            registers regs;
            regs.cx = window->x;
            regs.dx = window->y;
            regs.bx = 160;
            regs.al = colour::white | 0x20;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)versionInfo.c_str();
            call(0x00451025, regs);
        }
    }
}
