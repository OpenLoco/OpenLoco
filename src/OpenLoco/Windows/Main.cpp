#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::Main
{
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;

    namespace widx
    {
        enum
        {
            viewport
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 0, 0 }, widget_type::viewport, 0, 0xFFFFFFFE),
        widgetEnd(),
    };

    static window_event_list _events;

    static void initEvents();

    // 0x00438A6C, 0x0043CB9F
    void open()
    {
        initEvents();

        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        _widgets[widx::viewport].bottom = uiHeight;
        _widgets[widx::viewport].right = uiWidth;
        auto window = WindowManager::createWindow(
            WindowType::main,
            { 0, 0 },
            Gfx::ui_size_t(uiWidth, uiHeight),
            Ui::WindowFlags::stick_to_back,
            &_events);
        window->widgets = _widgets;
        gCurrentRotation = 0;
        ViewportManager::create(
            window,
            0,
            { window->x, window->y },
            { window->width, window->height },
            ZoomLevel::full,
            { (Map::map_rows * Map::tile_size) / 2 - 1, (Map::map_rows * Map::tile_size) / 2 - 1, 480 });
    }

    // 0x0043B2E4
    static void draw(Ui::window* const window, Gfx::Context* const dpi)
    {
        window->drawViewports(dpi);
    }

    static void initEvents()
    {
        _events.draw = draw;
    }
}
