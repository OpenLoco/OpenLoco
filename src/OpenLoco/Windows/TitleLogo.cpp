#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::interop;

namespace OpenLoco::ui::windows
{
    static const Gfx::ui_size_t window_size = { 298, 170 };

    namespace widx
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
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi);

    ui::window* openTitleLogo()
    {
        _events.on_mouse_up = onMouseUp;
        _events.draw = draw;

        auto window = OpenLoco::ui::WindowManager::createWindow(
            WindowType::title_logo,
            { 0, 0 },
            window_size,
            window_flags::stick_to_front | window_flags::transparent,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = 1 << widx::logo;

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::grey);
        window->colours[1] = Colour::translucent(Colour::grey);

        return window;
    }

    // 0x00439298
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        Gfx::drawImage(dpi, window->x, window->y, image_ids::locomotion_logo);
    }

    // 0x004392AD
    static void onMouseUp(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::logo:
                about::open();
                break;
        }
    }
}
