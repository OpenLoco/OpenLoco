#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

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

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, window_size, WidgetType::wt_9, WindowColour::secondary, -1),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex);
    static void draw(Ui::Window* window, Gfx::Context* context);

    Window* open()
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

        window->setColour(WindowColour::primary, Colour::translucent(Colour::saturated_green));
        window->setColour(WindowColour::secondary, Colour::translucent(Colour::saturated_green));

        return window;
    }

    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        // Draw widgets.
        window->draw(context);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[Widx::options_button].top + 2;
        Gfx::point_t origin = { x, y };

        Gfx::drawStringCentredWrapped(context, &origin, window->width, Colour::white, StringIds::outlined_wcolour2_stringid, (const char*)&StringIds::options);
    }

    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex)
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
