#include "../Window.h"
#include "../graphics/colours.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::windows::TitleVersionWindow
{
    static widget_t widgets[] = {
        widget_end()
    };

    static ui::window_event_list _events;

    static void draw(ui::Window* window, gfx::drawpixelinfo_t* dpi);

    Window* open()
    {
        auto width = 512;
        auto height = 16;
        auto window = openloco::ui::windowmgr::create_window(
            WindowType::titleVersion,
            8,
            ui::height() - height,
            width,
            height,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);
        window->widgets = widgets;

        _events.draw = draw;

        return window;
    }

    // 0x00439236
    static void draw(ui::Window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto versionInfo = get_version_info();
        gfx::draw_string(*dpi, window->x, window->y, colour::white | 0x20, versionInfo.c_str());
    }
}
