#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Intro.h"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::TitleOptions
{
    static constexpr Ui::Size kWindowSize = { 60, 15 };

    namespace Widx
    {
        enum
        {
            options_button
        };
    }

    static constexpr Widget _widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::buttonWithImage, WindowColour::secondary),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = WindowManager::createWindow(
            WindowType::titleOptions,
            Ui::Point(Ui::width() - kWindowSize.width, 0),
            kWindowSize,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            getEvents());

        window->setWidgets(_widgets);
        window->enabledWidgets = (1 << Widx::options_button);

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());

        return window;
    }

    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw widgets.
        window.draw(drawingCtx);

        int16_t x = window.x + window.width / 2;
        int16_t y = window.y + window.widgets[Widx::options_button].top + 2;
        Ui::Point origin = { x, y };

        auto argsBuf = FormatArgumentsBuffer{};
        auto args = FormatArguments{ argsBuf };
        args.push(StringIds::options);

        tr.drawStringCentredWrapped(origin, window.width, Colour::white, StringIds::outlined_wcolour2_stringid, args);
    }

    static void onMouseUp([[maybe_unused]] Window& window, WidgetIndex_t widgetIndex)
    {
        if (Intro::isActive())
        {
            return;
        }

        switch (widgetIndex)
        {
            case Widx::options_button:
                Ui::Windows::Options::open();
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
