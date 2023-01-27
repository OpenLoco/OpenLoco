#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleLogo
{
    static constexpr Ui::Size kWindowSize = { 298, 170 };

    namespace Widx
    {
        enum
        {
            logo
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::wt_3, WindowColour::primary),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseUp(Window& window, WidgetIndex_t widgetIndex);
    static void draw(Ui::Window& window, Gfx::RenderTarget* rt);

    Window* open()
    {
        _events.onMouseUp = onMouseUp;
        _events.draw = draw;

        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::title_logo,
            { 0, 0 },
            kWindowSize,
            WindowFlags::openQuietly | WindowFlags::transparent,
            &_events);

        window->widgets = _widgets;
        window->enabledWidgets = 1 << Widx::logo;

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::grey).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::grey).translucent());

        return window;
    }

    // 0x00439298
    static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(rt, window.x, window.y, ImageIds::locomotion_logo);
    }

    // 0x004392AD
    static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::logo:
                About::open();
                break;
        }
    }
}
