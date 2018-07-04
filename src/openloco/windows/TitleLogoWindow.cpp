#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::windows::TitleLogoWindow
{
    static const gfx::ui_size_t window_size = { 298, 170 };

    namespace widx
    {
        enum
        {
            logo
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, window_size, widget_type::wt_3, 0),
        widget_end(),
    };

    static window_event_list _events;

    static void onClick(Window *window, widget_index widgetIndex);
    static void draw(ui::Window* window, gfx::drawpixelinfo_t* dpi);

    ui::Window* open()
    {
        _events.onClick = onClick;
        _events.draw = draw;

        auto window = openloco::ui::windowmgr::create_window(
            WindowType::titleLogo,
            0,
            0,
            window_size.width,
            window_size.height,
            WindowFlags::stickToFront | WindowFlags::transparent,
            &_events);

        window->widgets = _widgets;
        window->setEnabledWidgets( widx::logo);

        window->initScrollWidgets();

        window->colours[0] = colour::translucent(colour::grey);
        window->colours[1] = colour::translucent(colour::grey);

        return window;
    }

    // 0x00439298
    static void draw(ui::Window* window, gfx::drawpixelinfo_t* dpi)
    {
        gfx::draw_image(dpi, window->x, window->y, image_ids::locomotion_logo);
    }

    // 0x004392AD
    static void onClick(Window *window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::logo:
                AboutWindow::open();
                break;
        }
    }
}
