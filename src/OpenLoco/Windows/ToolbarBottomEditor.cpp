#include "../EditorController.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
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

    static const uint16_t windowHeight = 32;

    static window_event_list _events;

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 200, 34 }, widget_type::wt_3, 0),
        makeWidget({ 2, 2 }, { 196, 30 }, widget_type::wt_9, 0),
        makeWidget({ 440, 0 }, { 200, 34 }, widget_type::wt_3, 0),
        makeWidget({ 442, 2 }, { 196, 30 }, widget_type::wt_9, 0),
        widgetEnd(),
    };

    static void initEvents();

    static std::map<EditorController::Step, string_id> _stepNames = {
        { EditorController::Step::objectSelection, StringIds::editor_step_object_selection },
        { EditorController::Step::landscapeEditor, StringIds::editor_step_landscape },
        { EditorController::Step::scenarioOptions, StringIds::editor_step_options },
        { EditorController::Step::saveScenario, StringIds::editor_step_save },
    };

    // 0x0043CE21
    static void prepareDraw(window* self)
    {
        self->widgets[widx::next_button].type = widget_type::wt_9;
        self->widgets[widx::next_frame].type = widget_type::wt_3;

        if (EditorController::canGoBack())
        {
            self->widgets[widx::previous_button].type = widget_type::wt_9;
            self->widgets[widx::previous_frame].type = widget_type::wt_3;
        }
        else
        {
            self->widgets[widx::previous_button].type = widget_type::none;
            self->widgets[widx::previous_frame].type = widget_type::none;
        }

        // 0x0043CDD1
        self->widgets[widx::next_frame].right = self->width - 1;
        self->widgets[widx::next_frame].left = self->width - 1 - 2 - 195 - 2;
        self->widgets[widx::next_button].left = self->widgets[widx::next_frame].left + 2;
        self->widgets[widx::next_button].right = self->widgets[widx::next_frame].right - 2;
    }

    // 0x0043CE65
    static void draw(window* self, Gfx::drawpixelinfo_t* ctx)
    {
        widget_t& previous = self->widgets[widx::previous_frame];
        widget_t& next = self->widgets[widx::next_frame];

        if (EditorController::canGoBack())
        {
            Gfx::drawRect(ctx, previous.left + self->x, previous.top + self->y, previous.width(), previous.height(), 0x2000000 | 52);
        }
        Gfx::drawRect(ctx, next.left + self->x, next.top + self->y, next.width(), next.height(), 0x2000000 | 52);

        self->draw(ctx);

        if (EditorController::canGoBack())
        {
            Gfx::drawRectInset(ctx, previous.left + self->x + 1, previous.top + self->y + 1, previous.width() - 2, previous.height() - 2, self->colours[1], 0x30);
        }
        Gfx::drawRectInset(ctx, next.left + self->x + 1, next.top + self->y + 1, next.width() - 2, next.height() - 2, self->colours[1], 0x30);

        Gfx::drawStringCentred(*ctx, (previous.right + next.left) / 2 + self->x, self->y + self->height - 12, Colour::opaque(self->colours[2]) | Colour::outline_flag, _stepNames[EditorController::getCurrentStep()]);

        if (EditorController::canGoBack())
        {
            Gfx::drawImage(ctx, self->x + previous.left + 6, self->y + previous.top + 6, ImageIds::step_back);
            int x = (previous.left + 30 + previous.right) / 2;
            int y = previous.top + 6;
            colour_t textColour = Colour::opaque(self->colours[1]);
            if (Input::isHovering(self->type, self->number, widx::previous_button))
            {
                textColour = Colour::white;
            }
            Gfx::drawStringCentred(*ctx, self->x + x, self->y + y, textColour, StringIds::editor_previous_step);
            Gfx::drawStringCentred(*ctx, self->x + x, self->y + y + 10, textColour, _stepNames[EditorController::getPreviousStep()]);
        }
        Gfx::drawImage(ctx, self->x + next.right - 29, self->y + next.top + 4, ImageIds::step_forward);
        int x = next.left + (next.width() - 31) / 2;
        int y = next.top + 6;
        colour_t textColour = Colour::opaque(self->colours[1]);
        if (Input::isHovering(self->type, self->number, widx::next_button))
        {
            textColour = Colour::white;
        }
        Gfx::drawStringCentred(*ctx, self->x + x, self->y + y, textColour, StringIds::editor_next_step);
        Gfx::drawStringCentred(*ctx, self->x + x, self->y + y + 10, textColour, _stepNames[EditorController::getNextStep()]);
    }

    // 0x0043D0ED
    static void onMouseUp(window*, widget_index i)
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

    // 0x0043CCCD
    void open()
    {
        initEvents();

        Gfx::point_t origin = Gfx::point_t(0, Ui::height() - windowHeight);
        Gfx::ui_size_t windowSize = Gfx::ui_size_t(Ui::width(), windowHeight);
        auto window = WindowManager::createWindow(
            WindowType::editorToolbar,
            origin,
            windowSize,
            WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background,
            &_events);
        window->widgets = _widgets;
        window->setVisible(widx::previous_button, widx::previous_frame, widx::next_frame, widx::next_button);
        window->var_854 = 0;
        window->initScrollWidgets();
        window->colours[0] = Colour::translucent(Colour::saturated_green);
        window->colours[1] = Colour::translucent(Colour::saturated_green);
        window->colours[2] = Colour::translucent(Colour::saturated_green);
    }

    static void initEvents()
    {
        _events.on_mouse_up = onMouseUp;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
    }
}
