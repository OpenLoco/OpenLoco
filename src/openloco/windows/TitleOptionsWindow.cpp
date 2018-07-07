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
using namespace openloco::ui;

namespace openloco::windows::TitleOptionsWindow
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

    static void onClick(Window* window, widget_index widgetIndex);
    static void draw(ui::Window* window, gfx::GraphicsContext* context);

    Window* open()
    {
        _events.onClick = onClick;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            ui::width() - window_size.width,
            0,
            window_size.width,
            window_size.height,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->setEnabledWidgets(widx::options_button);

        window->initScrollWidgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);

        return window;
    }

    static void draw(ui::Window* window, gfx::GraphicsContext* context)
    {
        // Draw widgets.
        window->draw(context);

        int16_t x = window->x + window->width / 2;
        int16_t y = window->y + window->widgets[widx::options_button].top + 2;
        gfx::point_t origin = { x, y };

        gfx::draw_string_centred_wrapped(context, &origin, window->width, colour::white, string_ids::outlined_wcolour2_stringid2, (const char*)&string_ids::options);
    }

    static void onClick(Window* window, widget_index widgetIndex)
    {
        if (intro::is_active())
        {
            return;
        }

        switch (widgetIndex)
        {
            case widx::options_button:
                OptionsWindow::open();
                break;
        }
    }
}
