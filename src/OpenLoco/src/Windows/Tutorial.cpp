#include "Tutorial.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::Tutorial
{
    enum Widx
    {
        frame,
    };

    static constexpr Ui::Size kWindowSize = { 140, 29 };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::wt_3, WindowColour::primary),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    // 0x00438CAE
    Window* open()
    {
        auto window = WindowManager::createWindow(
            WindowType::tutorial,
            Ui::Point(kWindowSize.width, Ui::height() - 27),
            Ui::Size(Ui::width() - 280, 27),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());

        window->setWidgets(widgets);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, AdvancedColour(skin->colour_06).translucent());
            window->setColour(WindowColour::secondary, AdvancedColour(skin->colour_07).translucent());
        }

        return window;
    }

    // 0x00439B3D
    static void prepareDraw(Window& self)
    {
        self.widgets[Widx::frame].right = self.width - 1;
    }

    // 0x00439B4A
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        static constexpr StringId titleStringIds[] = {
            StringIds::tutorial_1_title,
            StringIds::tutorial_2_title,
            StringIds::tutorial_3_title,
        };

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto tutorialNumber = OpenLoco::Tutorial::getTutorialNumber();

        FormatArguments args{};
        args.push(titleStringIds[tutorialNumber]);

        auto& widget = self.widgets[Widx::frame];
        auto point = Point(self.x + widget.midX(), self.y + widget.top + 4);
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::tutorial_text, &args);

        point.y += 10;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::tutorial_control);
    }

    static constexpr WindowEventList kEvents = {
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
