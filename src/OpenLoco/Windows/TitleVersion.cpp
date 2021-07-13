#include "../Graphics/Colour.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include "../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleVersion
{
    static Widget widgets[] = {
        widgetEnd()
    };

    static Ui::WindowEventList _events;

    static void draw(Ui::Window* window, Gfx::Context* context);

    Window* open()
    {
        auto width = 512;
        auto height = 16;
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::openLocoVersion,
            Gfx::point_t(8, Ui::height() - height),
            Gfx::ui_size_t(width, height),
            WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background | WindowFlags::flag_6,
            &_events);
        window->widgets = widgets;

        _events.prepare_draw = (void (*)(Ui::Window*))0x0042A035;
        _events.draw = draw;

        return window;
    }

    // 0x00439236
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        auto versionInfo = getVersionInfo();
        Gfx::drawString(context, window->x, window->y, Colour::white | FormatFlags::textflag_5, (void*)versionInfo.c_str());
    }
}
