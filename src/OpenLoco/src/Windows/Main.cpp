#include "Config.h"
#include "Graphics/Gfx.h"
#include "Map/Tile.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "Widget.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::Main
{
    static loco_global<bool, 0x00F2533F> _showingGridlines;
    static loco_global<bool, 0x0112C2E1> _showingDirectionArrows;

    namespace widx
    {
        enum
        {
            viewport
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 0, 0 }, WidgetType::viewport, WindowColour::primary, Widget::kContentUnk),
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
            { (World::kMapRows * World::kTileSize) / 2 - 1, (World::kMapRows * World::kTileSize) / 2 - 1, 480 });
    }

    // 0x0043B2E4
    static void draw(Ui::Window& window, Gfx::RenderTarget* const rt)
    {
        window.drawViewports(rt);
    }

    static void initEvents()
    {
        _events.draw = draw;
    }

    // 0x00468FD3
    void showGridlines()
    {
        if (_showingGridlines)
        {
            return;
        }

        auto window = WindowManager::getMainWindow();
        if (window == nullptr || window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
        {
            return;
        }

        window->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
        window->invalidate();
        _showingGridlines = true;
    }

    // 0x00468FFE
    void hideGridlines()
    {
        if (!_showingGridlines)
        {
            return;
        }

        auto window = WindowManager::getMainWindow();
        if (window == nullptr || !window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
        {
            return;
        }

        window->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;
        window->invalidate();
        _showingGridlines = false;
    }

    void resetGridlines()
    {
        _showingGridlines = false;
    }

    // 0x004793C4
    void showDirectionArrows()
    {
        if (_showingDirectionArrows)
        {
            return;
        }

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr || mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
        {
            return;
        }

        mainWindow->viewports[0]->flags |= ViewportFlags::one_way_direction_arrows;
        mainWindow->invalidate();
        _showingDirectionArrows = true;
    }

    // 0x004793EF
    void hideDirectionArrows()
    {
        if (!_showingDirectionArrows)
        {
            return;
        }

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr || !mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
        {
            return;
        }

        mainWindow->viewports[0]->flags &= ~ViewportFlags::one_way_direction_arrows;
        mainWindow->invalidate();
        _showingDirectionArrows = false;
    }

    void resetDirectionArrows()
    {
        _showingDirectionArrows = false;
    }
}
