#include "../graphics/colours.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static void paint(ui::window* window, gfx::drawpixelinfo_t* dpi);

    static loco_global<window_event_list[1], 0x004f9f3c> _events;

    window* open_title_exit()
    {
        _events[0].draw = paint;

        auto window = openloco::ui::windowmgr::create_window(
            window_type::title_exit,
            ui::width() - 40,
            ui::height() - 28,
            40,
            28,
            (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6),
            (window_event_list*)&_events[0]);
        window->widgets = (ui::widget_t*)0x00509e58;
        window->enabled_widgets = (1 << 0);

        window->init_scroll_widgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);

        return window;
    }

    // 0x00439236
    static void paint(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        window->draw(dpi);

        // draw_string_centred_wrapped(uint16_t string_id@<bx>, 16 center_x@<cx>, 16 center_y@<cx>, void* args@<esi>)
        {
            registers regs;
            regs.cx = window->x + window->width / 2;
            regs.dx = window->y + window->widgets[0].top + 8;
            regs.bx = string_ids::title_exit_game;
            regs.al = 0;
            regs.esi = 0x112c826; // common format args
            regs.edi = (uint32_t)&dpi;
            call(0x00494ECF, regs);
        }
    }

    // 0x00439268
    static void on_mouse_up(window* window, uint16_t widget_index)
    {
        if (intro::state() != intro::intro_state::none)
        {
            return;
        }

        switch (widget_index)
        {
            case 0:
                // do_game_command
                {
                    registers regs;
                    regs.bl = 1;
                    regs.dl = 0;
                    regs.di = 2;
                    regs.esi = 21;
                    call(0x00431315, regs);
                }
                break;
        }
    }
}
