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
    static const Ui::Size window_size = { 60, 15 };

    namespace Widx
    {
        enum
        {
            options_button
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, window_size, WidgetType::buttonWithImage, WindowColour::secondary, -1),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex);
    static void draw(Ui::Window* window, Gfx::Context* context);

    Window* open()
    {
        _events.onMouseUp = onMouseUp;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            Ui::Point(Ui::width() - window_size.width, 0),
            window_size,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::options_button);

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());

        return window;
    }

    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        // Draw widgets.
        window->draw(context);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[Widx::options_button].top + 2;
        Ui::Point origin = { x, y };

        Gfx::drawStringCentredWrapped(*context, origin, window->width, Colour::white, StringIds::outlined_wcolour2_stringid, (const char*)&StringIds::options);
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
