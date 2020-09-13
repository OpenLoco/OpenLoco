#include "../Graphics/Colour.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Window.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static widget_t widgets[] = {
        { widget_type::end, 0, 0, 0, 0, 0, { 0 }, 0 }
    };

    static ui::window_event_list _events;

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* openTitleVersion()
    {
        auto width = 512;
        auto height = 16;
        auto window = openloco::ui::WindowManager::createWindow(
            WindowType::openLocoVersion,
            gfx::point_t(8, ui::height() - height),
            gfx::ui_size_t(width, height),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);
        window->widgets = widgets;

        _events.prepare_draw = (void (*)(ui::window*))0x0042A035;
        _events.draw = draw;

        return window;
    }

    // 0x00439236
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto versionInfo = getVersionInfo();
        gfx::drawString(dpi, window->x, window->y, colour::white | format_flags::textflag_5, (void*)versionInfo.c_str());
    }
}
