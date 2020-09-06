#include "../GameCommands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../Intro.h"
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
        makeWidget({ 0, 0 }, window_size, widget_type::wt_9, 1, -1),
        widgetEnd(),
    };

    static window_event_list _events;

    static void onMouseUp(window* window, widget_index widgetIndex);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* open()
    {
        _events.on_mouse_up = onMouseUp;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            gfx::point_t(ui::width() - window_size.width, 0),
            window_size,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::options_button);

        window->initScrollWidgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);

        return window;
    }

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[widx::options_button].top + 2;
        gfx::point_t origin = { x, y };

        gfx::drawStringCentredWrapped(dpi, &origin, window->width, colour::white, string_ids::outlined_wcolour2_stringid, (const char*)&string_ids::options);
    }

    static void onMouseUp(window* window, widget_index widgetIndex)
    {
        if (intro::isActive())
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
