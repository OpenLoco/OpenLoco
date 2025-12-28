#include "Graphics/Colour.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/ImageIds.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/WindowManager.h"

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

    static constexpr auto _widgets = makeWidgets(
        Widgets::Wt3Widget({ 0, 0 }, kWindowSize, WindowColour::primary)

    );

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::title_logo,
            { 0, 0 },
            kWindowSize,
            WindowFlags::openQuietly | WindowFlags::transparent,
            getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::grey).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::grey).translucent());

        return window;
    }

    // 0x00439298
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        drawingCtx.drawImage(window.x, window.y, ImageIds::locomotion_logo);
    }

    // 0x004392AD
    static void onMouseUp([[maybe_unused]] Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case Widx::logo:
                About::open();
                break;
        }
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
