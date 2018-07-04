#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::windows::TitleExitWindow
{
    static const gfx::ui_size_t window_size = { 40, 28 };

    namespace widx
    {
        enum
        {
            exit_button
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, window_size, widget_type::wt_9, 1, -1, string_ids::title_menu_exit_from_game),
        widget_end(),
    };

    static window_event_list _events;

    static void onClick(Window* window, widget_index widgetIndex);
    static void draw(ui::Window* window, gfx::drawpixelinfo_t* dpi);

    Window* open()
    {
        _events.onClick = onClick;
        _events.draw = draw;

        auto window = openloco::ui::windowmgr::create_window(
            WindowType::titleExit,
            ui::width() - window_size.width,
            ui::height() - window_size.height,
            window_size.width,
            window_size.height,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->setEnabledWidgets(widx::exit_button);

        window->initScrollWidgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);

        return window;
    }

    // 0x00439236
    static void draw(ui::Window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[widx::exit_button].top + 8;
        gfx::point_t origin = { x, y };
        gfx::draw_string_centred_wrapped(dpi, &origin, window->width, colour::black, string_ids::title_exit_game);
    }

    // 0x00439268
    static void onClick(Window* window, widget_index widgetIndex)
    {
        if (intro::is_active())
        {
            return;
        }

        switch (widgetIndex)
        {
            case widx::exit_button:
                game_commands::do_21(1, 0, 2);
                break;
        }
    }
}
