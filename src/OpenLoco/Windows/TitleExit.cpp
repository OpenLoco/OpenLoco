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

namespace OpenLoco::Ui::Windows::TitleExit
{
    static const Ui::Size window_size = { 40, 28 };

    namespace Widx
    {
        enum
        {
            exit_button
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, window_size, WidgetType::buttonWithImage, WindowColour::secondary, -1, StringIds::title_menu_exit_from_game),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex);
    static void prepareDraw(Ui::Window* self);
    static void draw(Ui::Window* window, Gfx::Context* context);

    Window* open()
    {
        _events.onMouseUp = onMouseUp;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;

        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::titleExit,
            Ui::Point(Ui::width() - window_size.width, Ui::height() - window_size.height),
            window_size,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::exit_button);

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());

        return window;
    }

    static void prepareDraw(Ui::Window* self)
    {
        auto exitString = StringManager::getString(StringIds::title_exit_game);
        self->width = Gfx::getStringWidthNewLined(exitString) + 10;
        self->x = Ui::width() - self->width;
        self->widgets[Widx::exit_button].right = self->width;
    }

    // 0x00439236
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        // Draw widgets.
        window->draw(context);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[Widx::exit_button].top + 8;
        Ui::Point origin = { x, y };
        Gfx::drawStringCentredWrapped(*context, origin, window->width, Colour::black, StringIds::title_exit_game);
    }

    // 0x00439268
    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex)
    {
        if (Intro::isActive())
        {
            return;
        }

        switch (widgetIndex)
        {
            case Widx::exit_button:
                // Exit to desktop
                GameCommands::do_21(0, 2);
                break;
        }
    }
}
