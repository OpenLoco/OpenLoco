#include "Config.h"
#include "Graphics/Gfx.h"
#include "Map/Tile.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Interop/Interop.hpp>

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
        makeWidget({ 0, 0 }, { 0, 0 }, WidgetType::viewport, WindowColour::primary, Widget::kContentUnk),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    // 0x00438A6C, 0x0043CB9F
    void open()
    {
        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        _widgets[widx::viewport].bottom = uiHeight;
        _widgets[widx::viewport].right = uiWidth;

        auto window = WindowManager::createWindow(
            WindowType::main,
            { 0, 0 },
            Ui::Size(uiWidth, uiHeight),
            Ui::WindowFlags::stickToBack,
            getEvents());

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

    // 0x00468FD3
    void showGridlines()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr || window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
            return;

        window->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
        window->invalidate();
    }

    // 0x00468FFE
    void hideGridlines()
    {
        if (!Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
        {
            auto window = WindowManager::getMainWindow();
            if (window == nullptr || !window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
                return;

            window->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;
            window->invalidate();
        }
    }

    // 0x004793C4
    void showDirectionArrows()
    {
        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr || mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
            return;

        mainWindow->viewports[0]->flags |= ViewportFlags::one_way_direction_arrows;
        mainWindow->invalidate();
    }

    // 0x004793EF
    void hideDirectionArrows()
    {
        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr || !mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
            return;

        mainWindow->viewports[0]->flags &= ~ViewportFlags::one_way_direction_arrows;
        mainWindow->invalidate();
    }

    static constexpr WindowEventList kEvents = {
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
