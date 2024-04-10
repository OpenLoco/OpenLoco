#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleVersion
{
    static Widget widgets[] = {
        widgetEnd()
    };

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto width = 512;
        auto height = 16;
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::openLocoVersion,
            Ui::Point(8, Ui::height() - height),
            Ui::Size(width, height),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            getEvents());
        window->widgets = widgets;

        return window;
    }

    // 0x00439236
    static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto versionInfo = getVersionInfo();
        auto point = Point(window.x, window.y);
        drawingCtx.drawString(*rt, point, AdvancedColour(Colour::white).outline(), versionInfo.c_str());
    }

    static constexpr WindowEventList kEvents = {
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
