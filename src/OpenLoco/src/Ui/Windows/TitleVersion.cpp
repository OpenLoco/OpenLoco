#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::TitleVersion
{
    static const WindowEventList& getEvents();

    Window* open()
    {
        auto width = 512;
        auto height = 16;
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::openLocoVersion,
            { 8, Ui::height() - height },
            { width, height },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            getEvents());

        return window;
    }

    // 0x00439236
    static void draw([[maybe_unused]] Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        auto versionInfo = getVersionInfo();
        auto point = Point(0, 0);
        tr.drawString(point, AdvancedColour(Colour::white).outline(), versionInfo.c_str());
    }

    static constexpr WindowEventList kEvents = {
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
