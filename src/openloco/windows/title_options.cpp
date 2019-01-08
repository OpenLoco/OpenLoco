#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::title_options
{
    static const gfx::ui_size_t window_size = { 60, 15 };

    namespace widx
    {
        enum
        {
            options_button
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, window_size, widget_type::wt_9, 1, -1),
        widget_end(),
    };

    static window_event_list _events;

    static void on_mouse_up(window* window, widget_index widgetIndex);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* open()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            gfx::point_t(ui::width() - window_size.width, 0),
            window_size,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = (loco_ptr)_widgets;
        window->enabled_widgets = (1 << widx::options_button);

        window->init_scroll_widgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);

        return window;
    }

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->getWidget(widx::options_button)->top + 2;
        gfx::point_t origin = { x, y };

        gfx::draw_string_centred_wrapped(dpi, &origin, window->width, colour::white, string_ids::outlined_wcolour2_stringid2, (const char*)&string_ids::options);
    }

    static void on_mouse_up(window* window, widget_index widgetIndex)
    {
        if (intro::is_active())
        {
            return;
        }

        switch (widgetIndex)
        {
            case widx::options_button:
                ui::options::open();
                break;
        }
    }
}
