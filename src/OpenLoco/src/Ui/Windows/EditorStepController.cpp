#include "EditorController.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/Wt3Widget.h"

namespace OpenLoco::Ui::Windows::EditorStepController
{
    static constexpr Size kWindowSize = { 200, 32 };
    static constexpr Size kWindowSizeShrunk = { 200, 26 };

    enum widx
    {
        frame,
        button,
    };

    namespace Widx
    {
        constexpr WidgetId kFrame{ "frame" };
        constexpr WidgetId kButton{ "button" };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Wt3Widget(Widx::kFrame, { 0, 0 }, kWindowSize + Size{ 0, 2 }, WindowColour::primary),
        Widgets::ImageButton(Widx::kButton, { 2, 2 }, kWindowSize - Size{ 4, 2 }, WindowColour::primary)

    );

    static const WindowEventList& getEvents();

    // 0x0043CCCD
    void open(StepDirection direction)
    {
        const auto xPos = direction == StepDirection::previous ? 0 : Ui::width() - kWindowSize.width;
        const auto origin = Ui::Point(xPos, Ui::height() - kWindowSize.height);

        auto window = WindowManager::createWindow(
            WindowType::editorStepController,
            origin,
            kWindowSize,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());

        window->number = enumValue(direction);
        window->setWidgets(_widgets);
        window->var_854 = 0;
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::tertiary, AdvancedColour(Colour::mutedSeaGreen).translucent());
    }

    static bool isPreviousButton(Window& self)
    {
        return StepDirection(self.number) == StepDirection::previous;
    }

    // 0x0043CE21
    static void prepareDraw(Window& self)
    {
        const bool hidden = isPreviousButton(self) && !EditorController::canGoBack();
        self.widgets[widx::frame].hidden = hidden;
        self.widgets[widx::button].hidden = hidden;

        const auto& size = self.y > 0 ? kWindowSize : kWindowSizeShrunk;
        self.widgets[widx::frame].bottom = size.height;
        self.widgets[widx::button].bottom = size.height;
    }

    struct StepFrame
    {
        StringId label;
        uint32_t image;
        int32_t labelXOffset;
        int32_t imageXOffset;
    };

    static constexpr std::array kStepFrames = std::to_array<StepFrame>({
        { StringIds::editor_previous_step, ImageIds::step_back, (kWindowSize.width + 30) / 2, 6 },
        { StringIds::editor_next_step, ImageIds::step_forward, (kWindowSize.width - 31) / 2, kWindowSize.width - 29 },
    });

    // 0x0043CE65
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        if (self.widgets[widx::frame].hidden)
        {
            return;
        }

        // Draw frame
        auto& frame = self.widgets[widx::frame];
        drawingCtx.drawRect(frame.left, frame.top, frame.width(), frame.height(), enumValue(ExtColour::unk34), Gfx::RectFlags::transparent);
        self.draw(drawingCtx);
        drawingCtx.drawRectInset(frame.left + 1, frame.top + 1, frame.width() - 2, frame.height() - 2, self.getColour(WindowColour::secondary), Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillNone);

        const auto& layout = isPreviousButton(self) ? kStepFrames[0] : kStepFrames[1];
        const auto& labelOffset = Point{ layout.labelXOffset, self.y > 0 ? 6 : 8 };
        const auto& imageOffset = Point{ layout.imageXOffset, self.y > 0 ? 6 : 1 };

        auto imagePos = frame.position() + imageOffset;
        drawingCtx.drawImage(ZoomLevel::full, imagePos.x, imagePos.y, layout.image);

        auto textColour = self.getColour(WindowColour::secondary).opaque();
        if (Input::isHovering(self.type, self.number, widx::button))
        {
            textColour = Colour::white;
        }

        auto textPos = frame.position() + labelOffset;
        auto tr = Gfx::TextRenderer(drawingCtx);

        if (self.y != 0)
        {
            tr.drawStringCentred(textPos, textColour, layout.label);
            textPos.y += 10;
        }

        auto labelStep = isPreviousButton(self) ? EditorController::getPreviousStepString() : EditorController::getNextStepString();
        tr.drawStringCentred(textPos, textColour, labelStep);
    }

    // 0x0043D0ED
    static void onMouseUp(Window& self, [[maybe_unused]] WidgetIndex_t i, const WidgetId id)
    {
        if (id != Widx::kButton)
        {
            return;
        }

        if (isPreviousButton(self))
        {
            EditorController::goToPreviousStep();
        }
        else
        {
            EditorController::goToNextStep();
        }
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
