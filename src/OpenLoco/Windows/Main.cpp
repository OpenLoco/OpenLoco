#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::Main
{
    namespace widx
    {
        enum
        {
            viewport
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 0, 0 }, WidgetType::viewport, WindowColour::primary, 0xFFFFFFFE),
        widgetEnd(),
    };

    static WindowEventList _events;

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
            Ui::Size(uiWidth, uiHeight),
            Ui::WindowFlags::stickToBack,
            &_events);
        window->widgets = _widgets;
        WindowManager::setCurrentRotation(0);
        ViewportManager::create(
            window,
            0,
            { window->x, window->y },
            { window->width, window->height },
            ZoomLevel::full,
            { (Map::map_rows * Map::tile_size) / 2 - 1, (Map::map_rows * Map::tile_size) / 2 - 1, 480 });
    }

    // 0x0043B2E4
    static void draw(Ui::Window* const window, Gfx::Context* const context)
    {
        window->drawViewports(context);
    }

    static void initEvents()
    {
        _events.draw = draw;
    }
}
