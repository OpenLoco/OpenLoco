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
    static loco_global<int8_t, 0x00F2533F> _gridlinesState;
    static loco_global<uint8_t, 0x0112C2E1> _directionArrowsState;

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
        if (!_gridlinesState)
        {
            auto window = WindowManager::getMainWindow();
            if (window != nullptr)
            {
                if (!window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
                {
                    window->invalidate();
                }
                window->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
            }
        }
        _gridlinesState++;
    }

    // 0x00468FFE
    void hideGridlines()
    {
        _gridlinesState--;
        if (!_gridlinesState)
        {
            if (!Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
            {
                auto window = WindowManager::getMainWindow();
                if (window != nullptr)
                {
                    if (window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
                    {
                        window->invalidate();
                    }
                    window->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;
                }
            }
        }
    }

    // 0x004793C4
    void showDirectionArrows()
    {
        if (!_directionArrowsState)
        {
            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                if (!mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
                {
                    mainWindow->viewports[0]->flags |= ViewportFlags::one_way_direction_arrows;
                    mainWindow->invalidate();
                }
            }
        }
        _directionArrowsState++;
    }

    // 0x004793EF
    void hideDirectionArrows()
    {
        _directionArrowsState--;
        if (!_directionArrowsState)
        {
            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                if (mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
                {
                    mainWindow->viewports[0]->flags &= ~ViewportFlags::one_way_direction_arrows;
                    mainWindow->invalidate();
                }
            }
        }
    }

}
