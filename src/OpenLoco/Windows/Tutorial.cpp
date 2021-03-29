#include "../Tutorial.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::Tutorial
{
    enum Widx
    {
        frame,
    };

    constexpr Gfx::ui_size_t windowSize = { 140, 29 };

    widget_t widgets[] = {
        makeWidget({ 0, 0 }, windowSize, widget_type::wt_3, 0),
        widgetEnd(),
    };

    static window_event_list _events;

    static void initEvents();

    // 0x00438CAE
    window* open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::tutorial,
            Gfx::point_t(windowSize.width, Ui::height() - 27),
            Gfx::ui_size_t(Ui::width() - 280, 27),
            WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background,
            &_events);

        window->widgets = widgets;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = Colour::translucent(skin->colour_06);
            window->colours[1] = Colour::translucent(skin->colour_07);
        }

        return window;
    }

    // 0x00439B3D
    static void prepareDraw(window* self)
    {
        self->widgets[Widx::frame].right = self->width - 1;
    }

    // 0x00439B4A
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        static constexpr string_id titleStringIds[] = {
            StringIds::tutorial_1_title,
            StringIds::tutorial_2_title,
            StringIds::tutorial_3_title,
        };

        auto tutorialNumber = OpenLoco::Tutorial::getTutorialNumber();
        auto args = FormatArguments::common(titleStringIds[tutorialNumber]);

        auto& widget = self->widgets[Widx::frame];
        auto yPos = self->y + widget.top + 4;
        Gfx::drawStringCentred(*dpi, self->x + widget.mid_x(), yPos, Colour::black, StringIds::tutorial_text, &args);

        yPos += 10;
        Gfx::drawStringCentred(*dpi, self->x + widget.mid_x(), yPos, Colour::black, StringIds::tutorial_control, nullptr);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.prepare_draw = prepareDraw;
    }
}
