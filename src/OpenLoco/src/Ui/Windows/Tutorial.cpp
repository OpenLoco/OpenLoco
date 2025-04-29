#include "Tutorial.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::Tutorial
{
    enum Widx
    {
        frame,
    };

    static constexpr Ui::Size32 kWindowSize = { 140, 29 };

    static constexpr auto widgets = makeWidgets(
        Widgets::Wt3Widget({ 0, 0 }, kWindowSize, WindowColour::primary)

    );

    static const WindowEventList& getEvents();

    // 0x00438CAE
    Window* open()
    {
        auto window = WindowManager::createWindow(
            WindowType::tutorial,
            { kWindowSize.width, Ui::height() - 27 },
            { Ui::width() - 280, 27 },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());

        window->setWidgets(widgets);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, AdvancedColour(skin->mapTooltipObjectColour).translucent());
            window->setColour(WindowColour::secondary, AdvancedColour(skin->mapTooltipCargoColour).translucent());
        }

        return window;
    }

    // 0x00439B3D
    static void prepareDraw(Window& self)
    {
        self.widgets[Widx::frame].right = self.width - 1;
    }

    // 0x00439B4A
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        static constexpr StringId titleStringIds[] = {
            StringIds::tutorial_1_title,
            StringIds::tutorial_2_title,
            StringIds::tutorial_3_title,
        };

        auto tr = Gfx::TextRenderer(drawingCtx);

        auto tutorialNumber = OpenLoco::Tutorial::getTutorialNumber();

        FormatArguments args{};
        args.push(titleStringIds[tutorialNumber]);

        auto& widget = self.widgets[Widx::frame];
        auto point = Point(self.x + widget.midX(), self.y + widget.top + 4);
        tr.drawStringCentred(point, Colour::black, StringIds::tutorial_text, args);

        point.y += 10;
        tr.drawStringCentred(point, Colour::black, StringIds::tutorial_control);
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
