#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../openloco.h"
#include "../windowmgr.h"
#include "graphics/gfx.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{

    constexpr string_id STR_TOOLTIP_1567 = 1567;
    constexpr string_id STR_1568 = 1568;
    constexpr string_id STR_1569 = 1569;

    constexpr string_id STR_1717 = 1717;
    constexpr string_id STR_1718 = 1718;

    constexpr string_id STR_1934 = 1934;

    // 0x00438E0b (window *window@<esi>)
    static void event_26(window* window)
    {
        window->var_14 = 0;
        window->widgets[2].type = (uint8_t)ui::widget_type::widget_9;
        window->widgets[3].type = (uint8_t)ui::widget_type::widget_9;

        window->widgets[0].left = 0;
        window->widgets[0].right = 73;
        window->widgets[1].left = 74;
        window->widgets[1].right = 147;
        window->widgets[2].left = 148;
        window->widgets[2].right = 221;
        window->widgets[3].left = 222;
        window->widgets[3].right = 295;
        window->widgets[4].type = (uint8_t)ui::widget_type::widget_none;

        if (openloco::get_screen_flags() & screen_flags::unknown_2)
        {
            window->widgets[2].type = (uint8_t)ui::widget_type::widget_none;
            window->widgets[3].type = (uint8_t)ui::widget_type::widget_none;

            window->widgets[0].left = 74;
            window->widgets[0].right = 147;
            window->widgets[1].left = 148;
            window->widgets[1].right = 221;

            window->widgets[4].type = (uint8_t)ui::widget_type::widget_9;
            interface_skin_object* skin = objectmgr::get_interface_skin_object();
            window->widgets[4].image = skin->img + 188;
        }
    }

    // 0x00439298
    static void draw(window* window)
    {
        window->draw_widgets();

        if (window->widgets[0].type != 0)
        {
            int16_t x = window->widgets[0].left + window->x;
            int16_t y = window->widgets[0].top + window->y;

            uint32_t image_id = 3552;
            if ((addr<0x005233A8, uint8_t>() == 16) && (addr<0x005233AC, uint8_t>() == 0))
            {
                image_id = 3552 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(x, y, image_id);
            openloco::gfx::draw_image(x, y, 3547);
        }

        if (window->widgets[1].type != 0)
        {
            int16_t x = window->widgets[1].left + window->x;
            int16_t y = window->widgets[1].top + window->y;

            uint32_t image_id = 3552;
            if ((addr<0x005233A8, uint8_t>() == 16) && (addr<0x005233AC, uint8_t>() == 1))
            {
                image_id = 3552 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(x, y, image_id);
            openloco::gfx::draw_image(x, y, 3548);
        }

        if (window->widgets[2].type != 0)
        {
            int16_t x = window->widgets[2].left + window->x;
            int16_t y = window->widgets[2].top + window->y;

            uint32_t image_id = 3552;
            if ((addr<0x005233A8, uint8_t>() == 16) && (addr<0x005233AC, uint8_t>() == 2))
            {
                image_id = 3552 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(x, y, image_id);
            openloco::gfx::draw_image(x, y, 3549);
        }

        if (window->widgets[3].type != 0)
        {
            int16_t x = window->widgets[3].left + window->x;
            int16_t y = window->widgets[3].top + window->y;

            uint32_t image_id = 3608;
            if ((addr<0x005233A8, uint8_t>() == 16) && (addr<0x005233AC, uint8_t>() == 3))
            {
                image_id = 3584 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(x, y, image_id);
        }

        constexpr STR_2039 = 2039;

        {
            int16_t y = window->widgets[5].top + 3 + window->y;
            int16_t x = window->width / 2 + window->x;

            string_id string = STR_1568;

            if (false)
            {
                // 0x005177FA = char[512+1]
                // 0x00F254D0 player name
                0x5177FA [0] = '\0';
                std::strcpy((void*)0x5177FA, 0x00F254D0)
                    loco_global<0x112c826, uint16_t>()
                    = STR_2039;
                string = STR_1569;
            }

            registers regs;
            regs.al = 0;
            regs.bx = string;
            regs.cx = x;
            regs.dx = y;
            regs.bp = 292;        // color???
            regs.esi = 0x112c826; // common format args
            call(0x00494C36, regs);
        }
    }

    // 0x00439094
    static void event_1(uint16_t widget_index)
    {
        if (intro::state() != intro::intro_state::none)
        {
            return;
        }

        call(0x0046e328);
        switch (widget_index)
        {
            case 0:
                sub_4391DA();
                break;
            case 1:
                sub_4391E2();
                break;
            case 3:
                sub_43910A();
                break;
            case 4:
                sub_439163();
                break;
            case 5:
                sub_439102();
                break;
        }
    }

    // window * window@<esi>, widget * widget@<edi>, u16 index@<dx>
    // 0x004390D1
    static void event_4(uint16_t widget_index)
    {
        call(0x0046e328);
        switch (widget_index)
        {
            case 2:
                sub_439112();
                break;
        }
    }

    // 0x004390DD
    static void event_5(uint16_t widget_index)
    {
        call(0x0046e328);
        switch (widget_index)
        {
            case 2:
                sub_4391CC();
                break;
        }
    }

    // 0x004390ED
    static void event_20(uint16_t widget_index, char* input)
    {
        switch (widget_index)
        {
            case 4:
                sub_43918F(input);
                break;
        }
    }

    // 0x004390f8
    static void event_24()
    {
        addr<0x0052338a, uint16_t>() = 2000;
    }

    static void sub_439102()
    {
        call(0x0046e639); // window_multiplayer::open
    }

    static void sub_43910A()
    {
        call(0x0043D7DC); // show_scenario_editor
    }

    static void sub_439112()
    {
        // dropdownFormat[0] = STR_1879
        // dropdownFormat[1] = STR_1880
        // dropdownFormat[2] = STR_1881
        // al = translucent(window->colours[0])

        // cx = window->x + widget->left
        // dx = window->y + widget->top
        // bx = 0x8003 (flags and count?)
        // bp = widget->right - widget->left + 1
        // di = widget->bottom - widget->top + 1
        // call(0x4CCA6D, regs);
    }

    static void sub_439163()
    {
        windowmgr::close(window_type::multiplayer);

        {
            // open textinput
            registers regs;
            loco_global<0x112c82E, uint16_t>() = STR_1934;
            regs.eax = STR_1717;
            regs.ebx = STR_1718;
            regs.ecx = 0;
            call(0x004ce523, regs);
        }
    }

    static void sub_43918F(char string[512])
    {
        if (cl == 0)
        {
            return;
        }

        0x9c68e8 = 0;

        for (int i = 0; i < 32; i++)
        {
        }
    }

    static void sub_4391CC()
    {
        if (ax == -1)
        {
            return;
        }

        registers regs;
        regs.ax = ax;
        call(0x43c590, regs); // tutorial::start();
    }

    static void sub_4391DA()
    {
        call(0x443868); // scenario_select::open()
    }

    static void sub_4391E2()
    {
        // do_game_command
        {
            regs.bl = 1;
            regs.dl = 0;
            regs.di = 0;
            regx.esi = 21;
            call(0x00431315, regs);
        }
    }

    // 0x004391F9 (window *window@<esi>)
    static void event_7(window* window)
    {
        window->var_846++;

        if (intro::state() != intro::intro_state::none)
        {
            window->invalidate();
            return;
        }

        if (addr<0x00508F10, uint16_t>() & 0x300)
        {
            window->invalidate();
            return;
        }

        if (addr<0x0050C1AE, int32_t>() < 20)
        {
            window->invalidate();
            return;
        }

        auto multiplayer = windowmgr::find(window_type::multiplayer);
        if (multiplayer == nullptr)
        {
            call(0x0046e639); // window_multiplayer::open
        }
    }
}
