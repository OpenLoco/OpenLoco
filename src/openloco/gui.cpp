#include "gui.h"
#include "graphics/colours.h"
#include "interop/interop.hpp"
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
            0,
            0,
            uiWidth,
            uiHeight,
            ui::window_flags::stick_to_back,
            (ui::window_event_list*)0x004FA1F4);
        window->widgets = _mainWindowWidgets;
        addr<0x00e3f0b8, int32_t>() = 0; // gCurrentRotation?
        openloco::ui::viewportmgr::create(window, window->x, window->y, window->width, window->height, false, 0, 6143, 6143, 480);

        addr<0x00F2533F, int8_t>() = 0; // grid lines
        addr<0x0112C2e1, int8_t>() = 0;
        addr<0x009c86f8, int32_t>() = 0;
        addr<0x009c870C, int8_t>() = 0; // Something to do with tutorial
        addr<0x009c870D, int8_t>() = 0;
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

            window = WindowManager::createWindow(
                WindowType::topToolbar,
                0,
                0,
                uiWidth,
                28,
                ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
                (ui::window_event_list*)0x4fa180);
            window->widgets = (ui::widget_t*)0x509c34;
            window->enabled_widgets = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12);
            window->init_scroll_widgets();

            auto skin = openloco::objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_12;
                window->colours[1] = skin->colour_13;
                window->colours[2] = skin->colour_14;
                window->colours[3] = skin->colour_15;
            }

            window = WindowManager::createWindow(
                WindowType::playerInfoToolbar,
                0,
                uiHeight - 27,
                140,
                27,
                ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
                (ui::window_event_list*)0x4fa024);
            window->widgets = (ui::widget_t*)0x509d08;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4);
            window->var_854 = 0;
            window->init_scroll_widgets();

            if (skin != nullptr)
            {
                window->colours[0] = colour::translucent(skin->colour_16);
                window->colours[1] = colour::translucent(skin->colour_16);
            }

            window = WindowManager::createWindow(
                WindowType::timeToolbar,
                uiWidth - 140,
                uiHeight - 27,
                140,
                27,
                ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
                (ui::window_event_list*)0x4fa098);
            window->widgets = (ui::widget_t*)0x509d5c;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
            window->var_854 = 0;
            window->var_856 = 0;
            window->init_scroll_widgets();

            if (skin != nullptr)
            {
                window->colours[0] = colour::translucent(skin->colour_17);
                window->colours[1] = colour::translucent(skin->colour_17);
            }

            if (openloco::tutorial::state() != tutorial::tutorial_state::none)
            {

                window = WindowManager::createWindow(
                    WindowType::tutorial,
                    140,
                    uiHeight - 27,
                    uiWidth - 280,
                    27,
                    ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
                    (ui::window_event_list*)0x4fa10c);
                window->widgets = (ui::widget_t*)0x509de0;
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
            window->y = uiHeight - 27;
        }

        window = WindowManager::find(WindowType::timeToolbar);
        if (window)
        {
            window->y = uiHeight - 27;
            window->x = std::max(uiWidth, 640) - 140;
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
