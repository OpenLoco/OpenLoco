#include "../graphics/colours.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../window.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static widget_t widgets[] = {
        { widget_type::end, 0, 0, 0, 0, 0, { 0 }, 0 }
    };

    static ui::window_event_list _events;

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    window* open_title_version()
    {
        auto width = 512;
        auto height = 16;
        auto window = openloco::ui::WindowManager::createWindow(
            WindowType::openLocoVersion,
            gfx::point_t(8, ui::height() - height),
            gfx::ui_size_t(width, height),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);
        window->widgets = (loco_ptr)widgets;

        _events.prepare_draw = (void (*)(ui::window*))0x0042A035;
        _events.draw = draw;

        return window;
    }

    // 0x00439236
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto versionInfo = get_version_info();
        gfx::draw_string(dpi, window->x, window->y, colour::white | format_flags::textflag_5, (void*)versionInfo.c_str());
    }
}
