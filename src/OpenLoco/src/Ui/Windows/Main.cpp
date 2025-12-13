#include "Config.h"
#include "Entities/EntityManager.h"
#include "Graphics/Gfx.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ViewportWidget.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

namespace OpenLoco::Ui::Windows::Main
{
    namespace widx
    {
        enum
        {
            viewport
        };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Viewport({ 0, 0 }, { 0, 0 }, WindowColour::primary, Widget::kContentUnk)

    );

    static const WindowEventList& getEvents();

    // 0x00438A6C, 0x0043CB9F
    void open()
    {
        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        auto window = WindowManager::createWindow(
            WindowType::main,
            { 0, 0 },
            { uiWidth, uiHeight },
            Ui::WindowFlags::stickToBack,
            getEvents());

        window->setWidgets(_widgets);
        window->widgets[widx::viewport].bottom = uiHeight;
        window->widgets[widx::viewport].right = uiWidth;

        WindowManager::setCurrentRotation(0);
        ViewportManager::create(
            window,
            0,
            { window->x, window->y },
            { window->width, window->height },
            ZoomLevel::full,
            { (World::TileManager::getMapRows() * World::kTileSize) / 2 - 1, (World::TileManager::getMapRows() * World::kTileSize) / 2 - 1, 480 });
    }

    // 0x0043B2E4
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        window.draw(drawingCtx);
    }

    // 0x00468FD3
    void showGridlines()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr || window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
        {
            return;
        }

        window->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
        window->invalidate();
    }

    // 0x00468FFE
    void hideGridlines()
    {
        if (!Config::get().gridlinesOnLandscape)
        {
            auto window = WindowManager::getMainWindow();
            if (window == nullptr || !window->viewports[0]->hasFlags(ViewportFlags::gridlines_on_landscape))
            {
                return;
            }

            window->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;
            window->invalidate();
        }
    }

    // 0x004793C4
    void showDirectionArrows()
    {
        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr || mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
        {
            return;
        }

        mainWindow->viewports[0]->flags |= ViewportFlags::one_way_direction_arrows;
        mainWindow->invalidate();
    }

    // 0x004793EF
    void hideDirectionArrows()
    {
        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr || !mainWindow->viewports[0]->hasFlags(ViewportFlags::one_way_direction_arrows))
        {
            return;
        }

        mainWindow->viewports[0]->flags &= ~ViewportFlags::one_way_direction_arrows;
        mainWindow->invalidate();
    }

    void viewportFocusOnEntity(Window& main, EntityId targetEntity)
    {
        if (main.viewports[0] == nullptr)
        {
            return;
        }

        main.viewportConfigurations[0].viewportTargetSprite = targetEntity;
    }

    bool viewportIsFocusedOnEntity(const Window& main, EntityId targetEntity)
    {
        if (targetEntity == EntityId::null || main.viewports[0] == nullptr)
        {
            return false;
        }

        return main.viewportConfigurations[0].viewportTargetSprite == targetEntity;
    }

    bool viewportIsFocusedOnAnyEntity(const Window& main)
    {
        if (main.viewports[0] == nullptr)
        {
            return false;
        }

        return main.viewportConfigurations[0].viewportTargetSprite != EntityId::null;
    }

    // Stop following the followed entity, leaving the viewport centred on it.
    void viewportUnfocusFromEntity(Window& main)
    {
        if (main.viewports[0] == nullptr)
        {
            return;
        }

        if (main.viewportConfigurations[0].viewportTargetSprite == EntityId::null)
        {
            return;
        }

        auto entity = EntityManager::get<EntityBase>(main.viewportConfigurations[0].viewportTargetSprite);
        main.viewportConfigurations[0].viewportTargetSprite = EntityId::null;
        main.viewportCentreOnTile(entity->position);
    }

    static constexpr WindowEventList kEvents = {
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
