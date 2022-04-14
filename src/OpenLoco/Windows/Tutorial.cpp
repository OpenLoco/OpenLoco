#include "../Tutorial.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

namespace OpenLoco::Ui::Windows::Tutorial
{
    enum Widx
    {
        frame,
    };

    constexpr Ui::Size windowSize = { 140, 29 };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, windowSize, WidgetType::wt_3, WindowColour::primary),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void initEvents();

    // 0x00438CAE
    Window* open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::tutorial,
            Ui::Point(windowSize.width, Ui::height() - 27),
            Ui::Size(Ui::width() - 280, 27),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            &_events);

        window->widgets = widgets;
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
    static void prepareDraw(Window* self)
    {
        self->widgets[Widx::frame].right = self->width - 1;
    }

    // 0x00439B4A
    static void draw(Window* self, Gfx::Context* context)
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
        Gfx::drawStringCentred(*context, self->x + widget.mid_x(), yPos, Colour::black, StringIds::tutorial_text, &args);

        yPos += 10;
        Gfx::drawStringCentred(*context, self->x + widget.mid_x(), yPos, Colour::black, StringIds::tutorial_control, nullptr);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.prepareDraw = prepareDraw;
    }
}
