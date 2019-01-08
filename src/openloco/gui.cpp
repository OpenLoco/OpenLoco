#include "gui.h"
#include "graphics/colours.h"
#include "interop/interop.hpp"
#include "map/tile.h"
#include "objects/interface_skin_object.h"
#include "objects/objectmgr.h"
#include "openloco.h"
#include "tutorial.h"
#include "ui.h"
#include "ui/WindowManager.h"
#include "viewportmgr.h"
#include "window.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::gui
{

    loco_global<openloco::ui::widget_t[64], 0x00509c20> _mainWindowWidgets;

    // 0x00438A6C
    void init()
    {
        const int32_t uiWidth = ui::width();
        const int32_t uiHeight = ui::height();

        _mainWindowWidgets[0].bottom = uiHeight;
        _mainWindowWidgets[0].right = uiWidth;
        auto window = WindowManager::createWindow(
            WindowType::main,
            { 0, 0 },
            gfx::ui_size_t(uiWidth, uiHeight),
            ui::window_flags::stick_to_back,
            (ui::window_event_list*)0x004FA1F4);
        window->widgets = (loco_ptr) _mainWindowWidgets;
        addr<0x00e3f0b8, int32_t>() = 0; // gCurrentRotation?
        openloco::ui::viewportmgr::create(
            window,
            0,
            { window->x, window->y },
            { window->width, window->height },
            viewportmgr::ZoomLevel::full,
            { (map::map_rows * map::tile_size) / 2 - 1, (map::map_rows * map::tile_size) / 2 - 1, 480 });

        addr<0x00F2533F, int8_t>() = 0; // grid lines
        addr<0x0112C2e1, int8_t>() = 0;
        addr<0x009c870E, int8_t>() = 1;
        addr<0x009c870F, int8_t>() = 2;
        addr<0x009c8710, int8_t>() = 1;

        if (openloco::is_title_mode())
        {
            ui::windows::open_title_menu();
            ui::windows::open_title_exit();
            ui::windows::open_title_logo();
            ui::windows::open_title_version();
            ui::title_options::open();
        }
        else
        {
            windows::toolbar_top::open();

            window = WindowManager::createWindow(
                WindowType::playerInfoToolbar,
                gfx::point_t(0, uiHeight - 27),
                gfx::ui_size_t(140, 27),
                ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
                (ui::window_event_list*)0x4fa024);
            window->widgets = (loco_ptr)(loco_ptr)(ui::widget_t*)0x509d08;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4);
            window->var_854 = 0;
            window->init_scroll_widgets();

            auto skin = openloco::objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = colour::translucent(skin->colour_16);
                window->colours[1] = colour::translucent(skin->colour_16);
            }
            TimePanel::open();

            if (openloco::tutorial::state() != tutorial::tutorial_state::none)
            {

                window = WindowManager::createWindow(
                    WindowType::tutorial,
                    gfx::point_t(140, uiHeight - 27),
                    gfx::ui_size_t(uiWidth - 280, 27),
                    ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
                    (ui::window_event_list*)0x4fa10c);
                window->widgets = (loco_ptr)(ui::widget_t*)0x509de0;
                window->init_scroll_widgets();

                if (skin != nullptr)
                {
                    window->colours[0] = colour::translucent(skin->colour_06);
                    window->colours[1] = colour::translucent(skin->colour_07);
                }
            }
        }

        resize();
    }

    // 0x004392BD
    void resize()
    {
        const int32_t uiWidth = ui::width();
        const int32_t uiHeight = ui::height();

        if (openloco::is_editor_mode())
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
                widget_t*widgets = (widget_t*)(uintptr_t )window->widgets;
                widgets[0].right = uiWidth;
                widgets[0].bottom = uiHeight;
            }
            if (window->viewports[0])
            {
                viewport*vp = (viewport*)((uintptr_t )window->viewports[0]);
                vp->width = uiWidth;
                vp->height = uiHeight;
                vp->view_width = uiWidth << vp->zoom;
                vp->view_height = uiHeight << vp->zoom;
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
            window->y = uiHeight - 27;
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
