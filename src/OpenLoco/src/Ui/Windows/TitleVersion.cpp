#include "Graphics/Colour.h"
#include "Graphics/TextRenderer.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Version.hpp>

namespace OpenLoco::Ui::Windows::TitleVersion
{
    static const WindowEventList& getEvents();

    Window* open()
    {
        const auto kWidth = 512;
        const auto kHeight = 30;
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::openLocoVersion,
            { 8, Ui::height() - kHeight },
            { kWidth, kHeight },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::framedWidgets,
            getEvents());

        return window;
    }

    // 0x00439236
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        auto versionInfo = Version::getVersionInfo();
        auto point = Point(window.x, window.y);
        tr.drawString(point, AdvancedColour(Colour::white).outline(), versionInfo.c_str());

        auto platformInfo = Version::getPlatformInfo();
        point.y += 12;
        tr.drawString(point, AdvancedColour(Colour::white).outline(), platformInfo.c_str());
    }

    static constexpr WindowEventList kEvents = {
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
