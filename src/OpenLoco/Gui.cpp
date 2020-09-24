#include "Gui.h"
#include "Graphics/Colour.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::gui
{

    loco_global<OpenLoco::Ui::widget_t[64], 0x00509c20> _mainWindowWidgets;

    // 0x00438A6C
    void init()
    {
        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        _mainWindowWidgets[0].bottom = uiHeight;
        _mainWindowWidgets[0].right = uiWidth;
        auto window = WindowManager::createWindow(
            WindowType::main,
            { 0, 0 },
            Gfx::ui_size_t(uiWidth, uiHeight),
            Ui::window_flags::stick_to_back,
            (Ui::window_event_list*)0x004FA1F4);
        window->widgets = _mainWindowWidgets;
        addr<0x00e3f0b8, int32_t>() = 0; // gCurrentRotation?
        OpenLoco::Ui::viewportmgr::create(
            window,
            0,
            { window->x, window->y },
            { window->width, window->height },
            ZoomLevel::full,
            { (Map::map_rows * Map::tile_size) / 2 - 1, (Map::map_rows * Map::tile_size) / 2 - 1, 480 });

        addr<0x00F2533F, int8_t>() = 0; // grid lines
        addr<0x0112C2e1, int8_t>() = 0;
        addr<0x009c870E, int8_t>() = 1;
        addr<0x009c870F, int8_t>() = 2;
        addr<0x009c8710, int8_t>() = 1;

        if (OpenLoco::isTitleMode())
        {
            Ui::Windows::openTitleMenu();
            Ui::Windows::openTitleExit();
            Ui::Windows::openTitleLogo();
            Ui::Windows::openTitleVersion();
            Ui::TitleOptions::open();
        }
        else
        {
            Windows::ToolbarTop::Game::open();

            Windows::PlayerInfoPanel::open();
            TimePanel::open();

            if (OpenLoco::tutorial::state() != tutorial::tutorial_state::none)
            {

                window = WindowManager::createWindow(
                    WindowType::tutorial,
                    Gfx::point_t(140, uiHeight - 27),
                    Gfx::ui_size_t(uiWidth - 280, 27),
                    Ui::window_flags::stick_to_front | Ui::window_flags::transparent | Ui::window_flags::no_background,
                    (Ui::window_event_list*)0x4fa10c);
                window->widgets = (Ui::widget_t*)0x509de0;
                window->initScrollWidgets();

                auto skin = OpenLoco::ObjectManager::get<interface_skin_object>();
                if (skin != nullptr)
                {
                    window->colours[0] = Colour::translucent(skin->colour_06);
                    window->colours[1] = Colour::translucent(skin->colour_07);
                }
            }
        }

        resize();
    }

    // 0x004392BD
    void resize()
    {
        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        if (OpenLoco::isEditorMode())
        {
            call(0x43CD35);
            return;
        }

        auto window = WindowManager::getMainWindow();
        if (window)
        {
            window->width = uiWidth;
            window->height = uiHeight;
            if (window->widgets)
            {
                window->widgets[0].right = uiWidth;
                window->widgets[0].bottom = uiHeight;
            }
            if (window->viewports[0])
            {
                window->viewports[0]->width = uiWidth;
                window->viewports[0]->height = uiHeight;
                window->viewports[0]->view_width = uiWidth << window->viewports[0]->zoom;
                window->viewports[0]->view_height = uiHeight << window->viewports[0]->zoom;
            }
        }

        window = WindowManager::find(WindowType::topToolbar);
        if (window)
        {
            window->width = std::max(uiWidth, 640);
        }

        window = WindowManager::find(WindowType::playerInfoToolbar);
        if (window)
        {
            window->y = uiHeight - window->height;
        }

        window = WindowManager::find(WindowType::timeToolbar);
        if (window)
        {
            window->y = uiHeight - window->height;
            window->x = std::max(uiWidth, 640) - window->width;
        }

        window = WindowManager::find(WindowType::titleMenu);
        if (window)
        {
            window->x = uiWidth / 2 - 148;
            window->y = uiHeight - 117;
        }

        window = WindowManager::find(WindowType::titleExit);
        if (window)
        {
            window->x = uiWidth - 40;
            window->y = uiHeight - 28;
        }

        window = WindowManager::find(WindowType::openLocoVersion);
        if (window)
        {
            window->y = uiHeight - window->height;
        }

        window = WindowManager::find(WindowType::titleOptions);
        if (window)
        {
            window->x = uiWidth - window->width;
        }

        window = WindowManager::find(WindowType::tutorial);
        if (window)
        {
            if (tutorial::state() == tutorial::tutorial_state::none)
            {
                WindowManager::close(window);
            }
        }
    }
}
