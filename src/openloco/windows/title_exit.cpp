#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    namespace widx
    {
        enum
        {
            exit_button
        };
    }

    static widget_t _widgets[] = {
        make_widget( { 0, 0 }, { 40, 28 }, widget_type::wt_9, 1, -1, string_ids::title_menu_exit_from_game ),
        widget_end(),
    };

    static window_event_list _events;

    static void on_mouse_up(window* window, widget_index widgetIndex);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* open_title_exit()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.draw = draw;

        auto window = openloco::ui::windowmgr::create_window(
            window_type::title_exit,
            ui::width() - 40,
            ui::height() - 28,
            40,
            28,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::exit_button);

        window->init_scroll_widgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);

        return window;
    }

    // 0x00439236
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[0].top + 8;
        gfx::point_t origin = { x, y };
        gfx::draw_string_centred_wrapped(dpi, &origin, window->width, 0, string_ids::title_exit_game, nullptr);
    }

    // 0x00439268
    static void on_mouse_up(window* window, widget_index widgetIndex)
    {
        if (intro::state() != intro::intro_state::none)
        {
            return;
        }

        switch (widgetIndex)
        {
            case widx::exit_button:
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
