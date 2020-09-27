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

namespace OpenLoco::Ui::Windows
{
    static const Gfx::ui_size_t window_size = { 40, 28 };

    namespace Widx
    {
        enum
        {
            exit_button
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, window_size, widget_type::wt_9, 1, -1, StringIds::title_menu_exit_from_game),
        widgetEnd(),
    };

    static window_event_list _events;

    static void onMouseUp(window* window, widget_index widgetIndex);
    static void prepareDraw(Ui::window* self);
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi);

    window* openTitleExit()
    {
        _events.on_mouse_up = onMouseUp;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;

        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::titleExit,
            Gfx::point_t(Ui::width() - window_size.width, Ui::height() - window_size.height),
            window_size,
            WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << Widx::exit_button);

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::saturated_green);
        window->colours[1] = Colour::translucent(Colour::saturated_green);

        return window;
    }

    static void prepareDraw(Ui::window* self)
    {
        auto exitString = StringManager::getString(StringIds::title_exit_game);
        self->width = Gfx::getStringWidthNewLined(exitString) + 10;
        self->x = Ui::width() - self->width;
        self->widgets[Widx::exit_button].right = self->width;
    }

    // 0x00439236
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[Widx::exit_button].top + 8;
        Gfx::point_t origin = { x, y };
        Gfx::drawStringCentredWrapped(dpi, &origin, window->width, Colour::black, StringIds::title_exit_game);
    }

    // 0x00439268
    static void onMouseUp(window* window, widget_index widgetIndex)
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
