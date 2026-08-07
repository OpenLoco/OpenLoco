#include "EditorController.h"
#include "Graphics/TextRenderer.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::EditorStatusLine
{
    enum widx
    {
        frame,
    };

    namespace Widx
    {
        constexpr WidgetId kFrame{ "frame" };
    }

    static constexpr Ui::Size kWindowSize = { 140, 32 };

    static constexpr auto widgets = makeWidgets(
        Widgets::Wt3Widget(Widx::kFrame, { 0, 0 }, kWindowSize, WindowColour::primary)

    );

    static const WindowEventList& getEvents();

    void open()
    {
        auto window = WindowManager::createWindow(
            WindowType::editorStatusLine,
            { (Ui::width() - kWindowSize.width) / 2, Ui::height() - kWindowSize.height },
            kWindowSize,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());

        window->setWidgets(widgets);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::tertiary, AdvancedColour(Colour::mutedSeaGreen).translucent());
    }

    // 0x00439B3D
    static void prepareDraw(Window&)
    {
        // self.widgets[widx::frame].right = /
    }

    // 0x00439B4A
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto& widget = self.widgets[widx::frame];
        auto point = Point(widget.midX(), self.height - 12);

        auto tr = Gfx::TextRenderer(drawingCtx);
        auto stepStringId = EditorController::getCurrentStepString();
        tr.drawStringCentred(point, self.getColour(WindowColour::tertiary).opaque().outline(), stepStringId);
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
