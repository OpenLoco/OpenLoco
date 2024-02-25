#include "Drawing/SoftwareDrawingEngine.h"
#include "EditorController.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Widget.h"
#include <map>

namespace OpenLoco::Ui::Windows::ToolbarBottom::Editor
{
    enum widx
    {
        previous_frame,
        previous_button,
        next_frame,
        next_button,
    };

    static constexpr uint16_t kWindowHeight = 32;

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 200, 34 }, WidgetType::wt_3, WindowColour::primary),
        makeWidget({ 2, 2 }, { 196, 30 }, WidgetType::buttonWithImage, WindowColour::primary),
        makeWidget({ 440, 0 }, { 200, 34 }, WidgetType::wt_3, WindowColour::primary),
        makeWidget({ 442, 2 }, { 196, 30 }, WidgetType::buttonWithImage, WindowColour::primary),
        widgetEnd(),
    };

    static std::map<EditorController::Step, StringId> _stepNames = {
        { EditorController::Step::objectSelection, StringIds::editor_step_object_selection },
        { EditorController::Step::landscapeEditor, StringIds::editor_step_landscape },
        { EditorController::Step::scenarioOptions, StringIds::editor_step_options },
        { EditorController::Step::saveScenario, StringIds::editor_step_save },
    };

    // 0x0043CE21
    static void prepareDraw(Window& self)
    {
        self.widgets[widx::next_button].type = WidgetType::buttonWithImage;
        self.widgets[widx::next_frame].type = WidgetType::wt_3;

        if (EditorController::canGoBack())
        {
            self.widgets[widx::previous_button].type = WidgetType::buttonWithImage;
            self.widgets[widx::previous_frame].type = WidgetType::wt_3;
        }
        else
        {
            self.widgets[widx::previous_button].type = WidgetType::none;
            self.widgets[widx::previous_frame].type = WidgetType::none;
        }

        // 0x0043CDD1
        self.widgets[widx::next_frame].right = self.width - 1;
        self.widgets[widx::next_frame].left = self.width - 1 - 2 - 195 - 2;
        self.widgets[widx::next_button].left = self.widgets[widx::next_frame].left + 2;
        self.widgets[widx::next_button].right = self.widgets[widx::next_frame].right - 2;
    }

    // 0x0043CE65
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        Widget& previous = self.widgets[widx::previous_frame];
        Widget& next = self.widgets[widx::next_frame];

        if (EditorController::canGoBack())
        {
            drawingCtx.drawRect(*rt, previous.left + self.x, previous.top + self.y, previous.width(), previous.height(), enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);
        }
        drawingCtx.drawRect(*rt, next.left + self.x, next.top + self.y, next.width(), next.height(), enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);

        self.draw(rt);

        if (EditorController::canGoBack())
        {
            drawingCtx.drawRectInset(*rt, previous.left + self.x + 1, previous.top + self.y + 1, previous.width() - 2, previous.height() - 2, self.getColour(WindowColour::secondary), Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillNone);
        }
        drawingCtx.drawRectInset(*rt, next.left + self.x + 1, next.top + self.y + 1, next.width() - 2, next.height() - 2, self.getColour(WindowColour::secondary), Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillNone);

        drawingCtx.drawStringCentred(*rt, (previous.right + next.left) / 2 + self.x, self.y + self.height - 12, self.getColour(WindowColour::tertiary).opaque().outline(), _stepNames[EditorController::getCurrentStep()]);

        if (EditorController::canGoBack())
        {
            drawingCtx.drawImage(rt, self.x + previous.left + 6, self.y + previous.top + 6, ImageIds::step_back);
            int x = (previous.left + 30 + previous.right) / 2;
            int y = previous.top + 6;
            auto textColour = self.getColour(WindowColour::secondary).opaque();
            if (Input::isHovering(self.type, self.number, widx::previous_button))
            {
                textColour = Colour::white;
            }
            drawingCtx.drawStringCentred(*rt, self.x + x, self.y + y, textColour, StringIds::editor_previous_step);
            drawingCtx.drawStringCentred(*rt, self.x + x, self.y + y + 10, textColour, _stepNames[EditorController::getPreviousStep()]);
        }
        drawingCtx.drawImage(rt, self.x + next.right - 29, self.y + next.top + 4, ImageIds::step_forward);
        int x = next.left + (next.width() - 31) / 2;
        int y = next.top + 6;
        auto textColour = self.getColour(WindowColour::secondary).opaque();
        if (Input::isHovering(self.type, self.number, widx::next_button))
        {
            textColour = Colour::white;
        }
        drawingCtx.drawStringCentred(*rt, self.x + x, self.y + y, textColour, StringIds::editor_next_step);
        drawingCtx.drawStringCentred(*rt, self.x + x, self.y + y + 10, textColour, _stepNames[EditorController::getNextStep()]);
    }

    // 0x0043D0ED
    static void onMouseUp(Window&, WidgetIndex_t i)
    {
        switch (i)
        {
            case widx::previous_button:
                EditorController::goToPreviousStep();
                break;

            case widx::next_button:
                EditorController::goToNextStep();
                break;
        }
    }

    static constexpr WindowEventList _events = []() {
        return WindowEventList{
            .onMouseUp = onMouseUp,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };
    }();

    static const WindowEventList& getEvents()
    {
        return _events;
    }

    // 0x0043CCCD
    void open()
    {
        Ui::Point origin = Ui::Point(0, Ui::height() - kWindowHeight);
        Ui::Size windowSize = Ui::Size(Ui::width(), kWindowHeight);

        auto window = WindowManager::createWindow(
            WindowType::editorToolbar,
            origin,
            windowSize,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());

        window->widgets = _widgets;
        window->enabledWidgets = 1 << widx::previous_button | 1 << widx::previous_frame | 1 << widx::next_frame | 1 << widx::next_button;
        window->var_854 = 0;
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::tertiary, AdvancedColour(Colour::mutedSeaGreen).translucent());
    }
}
