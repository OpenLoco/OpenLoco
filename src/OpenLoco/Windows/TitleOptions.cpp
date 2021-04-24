#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleOptions
{
    static const Gfx::ui_size_t window_size = { 60, 15 };

    namespace Widx
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
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi);

    window* open()
    {
        _events.on_mouse_up = onMouseUp;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            Gfx::point_t(Ui::width() - window_size.width, 0),
            window_size,
            WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << Widx::options_button);

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::saturated_green);
        window->colours[1] = Colour::translucent(Colour::saturated_green);

        return window;
    }

    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[Widx::options_button].top + 2;
        Gfx::point_t origin = { x, y };

        Gfx::drawStringCentredWrapped(dpi, &origin, window->width, Colour::white, StringIds::outlined_wcolour2_stringid, (const char*)&StringIds::options);
    }

    static void onMouseUp(window* window, widget_index widgetIndex)
    {
        if (Intro::isActive())
        {
            return;
        }

        switch (widgetIndex)
        {
            case Widx::options_button:
                Ui::Windows::Options::open();
                break;
        }
    }
}
