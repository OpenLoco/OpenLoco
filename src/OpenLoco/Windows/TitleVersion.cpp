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
            Ui::Point(8, Ui::height() - height),
            Ui::Size(width, height),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);
        window->widgets = widgets;

        _events.draw = draw;

        return window;
    }

    // 0x00439236
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        auto versionInfo = getVersionInfo();
        Gfx::drawString(*context, window->x, window->y, AdvancedColour(Colour::white).outline(), (void*)versionInfo.c_str());
    }
}
