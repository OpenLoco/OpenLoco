#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows
{
    static const Gfx::ui_size_t window_size = { 298, 170 };

    namespace Widx
    {
        enum
        {
            logo
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, window_size, widget_type::wt_3, 0),
        widgetEnd(),
    };

    static window_event_list _events;

    static void onMouseUp(window* window, widget_index widgetIndex);
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi);

    Ui::window* openTitleLogo()
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
        window->setVisible(Widx::logo);

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::grey);
        window->colours[1] = Colour::translucent(Colour::grey);

        return window;
    }

    // 0x00439298
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        Gfx::drawImage(dpi, window->x, window->y, ImageIds::locomotion_logo);
    }

    // 0x004392AD
    static void onMouseUp(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::logo:
                About::open();
                break;
        }
    }
}
