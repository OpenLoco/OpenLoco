#include "gui.h"
#include "graphics/colours.h"
#include "interop/interop.hpp"
#include "objects/interface_skin_object.h"
#include "objects/objectmgr.h"
#include "openloco.h"
#include "tutorial.h"
#include "ui.h"
#include "viewportmgr.h"
#include "windowmgr.h"

namespace ui = openloco::ui;

namespace windowmgr = openloco::ui::windowmgr;
using window_type = openloco::ui::window_type;
using namespace openloco::interop;

namespace openloco::gui
{

    loco_global_array<openloco::ui::widget, 64, 0x00509c20> _mainWindowWidgets;

    // 0x00438A6C
    void init()
    {
        const int32_t uiWidth = ui::width();
        const int32_t uiHeight = ui::height();

        _mainWindowWidgets[0].bottom = uiHeight;
        _mainWindowWidgets[0].right = uiWidth;
        auto window = openloco::ui::windowmgr::create_window(
            window_type::main,
            0,
            0,
            uiWidth,
            uiHeight,
            (1 << 0),
            (void*)0x004FA1F4);
        window->widgets = _mainWindowWidgets;
        addr<0x00e3f0b8, int32_t>() = 0; // gCurrentRotation?
        openloco::ui::viewportmgr::create(window, window->x, window->y, window->width, window->height);

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
            window = openloco::ui::windowmgr::create_window(
                window_type::title_menu,
                (uiWidth - 296) / 2,
                uiHeight - 117,
                296,
                92,
                (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6),
                (void*)0x004f9ec8);
            window->widgets = (ui::widget*)0x00509df4;
            window->enabled_widgets = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);

            windowmgr::init_scroll_widgets(window);

            window->colours[0] = colour::translucent(colour::saturated_green);
            window->colours[1] = colour::translucent(colour::saturated_green);
            window->var_846 = 0;

            window = openloco::ui::windowmgr::create_window(
                window_type::title_exit,
                uiWidth - 40,
                uiHeight - 28,
                40,
                28,
                (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6),
                (void*)0x004f9f3c);
            window->widgets = (ui::widget*)0x00509e58;
            window->enabled_widgets = (1 << 0);

            windowmgr::init_scroll_widgets(window);

            window->colours[0] = colour::translucent(colour::saturated_green);
            window->colours[1] = colour::translucent(colour::saturated_green);

            window = openloco::ui::windowmgr::create_window(
                window_type::title_logo,
                0,
                0,
                298,
                170,
                1 << 1,
                (void*)0x004f9fb0);
            window->widgets = (ui::widget*)0x00509e6c;
            window->enabled_widgets = (1 << 0);

            windowmgr::init_scroll_widgets(window);

            window->var_42 |= 0x10;
            window->colours[0] = colour::translucent(colour::grey);
            window->colours[1] = colour::translucent(colour::grey);
        }
        else
        {

            window = openloco::ui::windowmgr::create_window(
                window_type::toolbar_top,
                0,
                0,
                uiWidth,
                28,
                (1 << 1) | (1 << 4) | (1 << 5),
                (void*)0x4fa180);
            window->widgets = (ui::widget*)0x509c34;
            window->enabled_widgets = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12);
            windowmgr::init_scroll_widgets(window);

            auto skin = openloco::objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colours_12[0];
                window->colours[1] = skin->colours_12[1];
                window->colours[2] = skin->colours_12[2];
                window->colours[3] = skin->colours_12[3];
            }

            window = openloco::ui::windowmgr::create_window(
                window_type::toolbar_player_info,
                0,
                uiHeight - 27,
                140,
                27,
                (1 << 1) | (1 << 4) | (1 << 5),
                (void*)0x4fa024);
            window->widgets = (ui::widget*)0x509d08;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4);
            window->var_854 = 0;
            windowmgr::init_scroll_widgets(window);

            if (skin != nullptr)
            {
                window->colours[0] = colour::translucent(skin->colours_12[4]);
                window->colours[1] = colour::translucent(skin->colours_12[4]);
            }

            window = openloco::ui::windowmgr::create_window(
                window_type::toolbar_time,
                uiWidth - 140,
                uiHeight - 27,
                140,
                27,
                (1 << 1) | (1 << 4) | (1 << 5),
                (void*)0x4fa098);
            window->widgets = (ui::widget*)0x509d5c;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
            window->var_854 = 0;
            window->var_856 = 0;
            windowmgr::init_scroll_widgets(window);

            if (skin != nullptr)
            {
                window->colours[0] = colour::translucent(skin->colours_12[5]);
                window->colours[1] = colour::translucent(skin->colours_12[5]);
            }

            if (openloco::tutorial::state() != tutorial::tutorial_state::none)
            {

                window = openloco::ui::windowmgr::create_window(
                    window_type::tutorial,
                    140,
                    uiHeight - 27,
                    uiWidth - 280,
                    27,
                    (1 << 1) | (1 << 4) | (1 << 5),
                    (void*)0x4fa10c);
                window->widgets = (ui::widget*)0x509de0;
                windowmgr::init_scroll_widgets(window);

                if (skin != nullptr)
                {
                    window->colours[0] = colour::translucent(skin->colours_06[0]);
                    window->colours[1] = colour::translucent(skin->colours_06[1]);
                }
            }
        }

        windowmgr::resize();
    }
}
