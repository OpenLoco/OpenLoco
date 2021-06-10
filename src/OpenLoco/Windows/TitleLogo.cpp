#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleLogo
{
    static const Gfx::ui_size_t window_size = { 298, 170 };

    namespace Widx
    {
        enum
        {
            logo
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, window_size, WidgetType::wt_3, 0),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex);
    static void draw(Ui::Window* window, Gfx::Context* context);

    Window* open()
    {
        _events.on_mouse_up = onMouseUp;
        _events.draw = draw;

        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::title_logo,
            { 0, 0 },
            window_size,
            WindowFlags::stick_to_front | WindowFlags::transparent,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = 1 << Widx::logo;

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::grey);
        window->colours[1] = Colour::translucent(Colour::grey);

        return window;
    }

    // 0x00439298
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        Gfx::drawImage(context, window->x, window->y, ImageIds::locomotion_logo);
    }

    // 0x004392AD
    static void onMouseUp(Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::logo:
                About::open();
                break;
        }
    }
}
