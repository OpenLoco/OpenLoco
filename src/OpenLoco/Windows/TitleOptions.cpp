#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::ui::title_options
{
    static const Gfx::ui_size_t window_size = { 60, 15 };

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
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi);

    window* open()
    {
        _events.on_mouse_up = onMouseUp;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            Gfx::point_t(ui::width() - window_size.width, 0),
            window_size,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::options_button);

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::saturated_green);
        window->colours[1] = Colour::translucent(Colour::saturated_green);

        return window;
    }

    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[widx::options_button].top + 2;
        Gfx::point_t origin = { x, y };

        Gfx::drawStringCentredWrapped(dpi, &origin, window->width, Colour::white, string_ids::outlined_wcolour2_stringid, (const char*)&string_ids::options);
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
