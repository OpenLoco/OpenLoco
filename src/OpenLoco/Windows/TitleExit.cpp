#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::interop;

namespace OpenLoco::ui::windows
{
    static const Gfx::ui_size_t window_size = { 40, 28 };

    namespace widx
    {
        enum
        {
            exit_button
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, window_size, widget_type::wt_9, 1, -1, string_ids::title_menu_exit_from_game),
        widgetEnd(),
    };

    static window_event_list _events;

    static void onMouseUp(window* window, widget_index widgetIndex);
    static void prepareDraw(ui::window* self);
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi);

    window* openTitleExit()
    {
        _events.on_mouse_up = onMouseUp;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;

        auto window = OpenLoco::ui::WindowManager::createWindow(
            WindowType::titleExit,
            Gfx::point_t(ui::width() - window_size.width, ui::height() - window_size.height),
            window_size,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::exit_button);

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::saturated_green);
        window->colours[1] = Colour::translucent(Colour::saturated_green);

        return window;
    }

    static void prepareDraw(ui::window* self)
    {
        auto exitString = stringmgr::getString(string_ids::title_exit_game);
        self->width = Gfx::getStringWidthNewLined(exitString) + 10;
        self->x = ui::width() - self->width;
        self->widgets[widx::exit_button].right = self->width;
    }

    // 0x00439236
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[widx::exit_button].top + 8;
        Gfx::point_t origin = { x, y };
        Gfx::drawStringCentredWrapped(dpi, &origin, window->width, Colour::black, string_ids::title_exit_game);
    }

    // 0x00439268
    static void onMouseUp(window* window, widget_index widgetIndex)
    {
        if (intro::isActive())
        {
            return;
        }

        switch (widgetIndex)
        {
            case widx::exit_button:
                // Exit to desktop
                game_commands::do_21(0, 2);
                break;
        }
    }
}
