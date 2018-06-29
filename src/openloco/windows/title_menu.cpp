#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    namespace widx
    {
        enum
        {
            scenario_list_btn,
            load_game_btn,
            tutorial_btn,
            scenario_editor_btn,
            chat_btn,
            multiplayer_toggle_btn,
        };
    }

    static widget_t _widgets[] = {
        make_widget( { 0, 0 }, { 74, 74 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_new_game ),
        make_widget( { 74, 0 }, { 74, 74 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_load_game ),
        make_widget( { 148, 0 }, { 74, 74 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_show_tutorial ),
        make_widget( { 222, 0 }, { 74, 74 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_scenario_editor ),
        make_widget( { 265, 47 }, { 31, 27 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_chat_tooltip ),
        make_widget( { 0, 74 }, { 296, 18 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_multiplayer_toggle_tooltip ),
        widget_end(),
    };

    static window_event_list _events;

    static void sub_439112();
    static void sub_4391CC();
    static void sub_43918F(char string[512]);
    static void sub_4391DA();
    static void sub_4391E2();
    static void sub_43910A();
    static void sub_439163(ui::window* callingWindow, widget_index callingWidget);
    static void sub_439102();
    static void sub_46E328();

    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_mouse_down(ui::window* window, widget_index widgetIndex);
    static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex);
    static void on_update(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void prepare_draw(ui::window* window);

    // static loco_global<window_event_list[1], 0x004f9ec8> _events;

    ui::window* open_title_menu()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = on_update;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;

        auto window = openloco::ui::windowmgr::create_window(
            window_type::title_menu,
            (ui::width() - 296) / 2,
            ui::height() - 117,
            296,
            92,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets =
            (1 << widx::scenario_list_btn) |
            (1 << widx::load_game_btn) |
            (1 << widx::tutorial_btn) |
            (1 << widx::scenario_editor_btn) |
            (1 << widx::chat_btn);

        window->init_scroll_widgets();

        window->colours[0] = colour::translucent(colour::saturated_green);
        window->colours[1] = colour::translucent(colour::saturated_green);
        window->var_846 = 0;

        return window;
    }

    // 0x00438E0B
    static void prepare_draw(ui::window* window)
    {
        window->disabled_widgets = 0;
        window->widgets[widx::tutorial_btn].type = ui::widget_type::wt_9;
        window->widgets[widx::scenario_editor_btn].type = ui::widget_type::wt_9;

        window->widgets[widx::scenario_list_btn].left = 0;
        window->widgets[widx::scenario_list_btn].right = 73;
        window->widgets[widx::load_game_btn].left = 74;
        window->widgets[widx::load_game_btn].right = 147;
        window->widgets[widx::tutorial_btn].left = 148;
        window->widgets[widx::tutorial_btn].right = 221;
        window->widgets[widx::scenario_editor_btn].left = 222;
        window->widgets[widx::scenario_editor_btn].right = 295;
        window->widgets[widx::chat_btn].type = ui::widget_type::none;

        if (openloco::get_screen_flags() & screen_flags::unknown_2)
        {
            window->widgets[widx::tutorial_btn].type = ui::widget_type::none;
            window->widgets[widx::scenario_editor_btn].type = ui::widget_type::none;

            window->widgets[widx::scenario_list_btn].left = 74;
            window->widgets[widx::scenario_list_btn].right = 147;
            window->widgets[widx::load_game_btn].left = 148;
            window->widgets[widx::load_game_btn].right = 221;

            window->widgets[widx::chat_btn].type = ui::widget_type::wt_9;
            interface_skin_object* skin = objectmgr::get<interface_skin_object>();
            window->widgets[widx::chat_btn].image = skin->img + 188;
        }
    }

    // 0x00438EC7
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        if (window->widgets[widx::scenario_list_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::scenario_list_btn].left + window->x;
            int16_t y = window->widgets[widx::scenario_list_btn].top + window->y;

            uint32_t image_id = 3552;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::scenario_list_btn))
            {
                image_id = 3552 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, 3547);
        }

        if (window->widgets[widx::load_game_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::load_game_btn].left + window->x;
            int16_t y = window->widgets[widx::load_game_btn].top + window->y;

            uint32_t image_id = 3552;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::load_game_btn))
            {
                image_id = 3552 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, 3548);
        }

        if (window->widgets[widx::tutorial_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::tutorial_btn].left + window->x;
            int16_t y = window->widgets[widx::tutorial_btn].top + window->y;

            uint32_t image_id = 3552;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::tutorial_btn))
            {
                image_id = 3552 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, 3549);
        }

        if (window->widgets[widx::scenario_editor_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::scenario_editor_btn].left + window->x;
            int16_t y = window->widgets[widx::scenario_editor_btn].top + window->y;

            uint32_t image_id = 3608;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::scenario_editor_btn))
            {
                image_id = 3584 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
        }

        {
            int16_t y = window->widgets[5].top + 3 + window->y;
            int16_t x = window->width / 2 + window->x;

            string_id string = string_ids::single_player_mode;

            // if (false)
            // {
            //    // 0x005177FA = char[512+1]
            //    // 0x00F254D0 player name
            //    0x5177FA [0] = '\0';
            //    std::strcpy((void*)0x5177FA, 0x00F254D0)
            //        loco_global<0x112c826, uint16_t>()
            //        = string_ids::buffer_2039;
            //    string = string_ids::two_player_mode_connected;
            // }

            draw_string_centred_clipped(*dpi, x, y, 292, 0, string, (char*)0x112c826);
        }
    }

    // 0x00439094
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        if (intro::state() != intro::intro_state::none)
        {
            return;
        }

        sub_46E328();

        switch (widgetIndex)
        {
            case widx::scenario_list_btn:
                sub_4391DA();
                break;
            case widx::load_game_btn:
                sub_4391E2();
                break;
            case widx::scenario_editor_btn:
                sub_43910A();
                break;
            case widx::chat_btn:
                sub_439163(window, widgetIndex);
                break;
            case widx::multiplayer_toggle_btn:
                sub_439102();
                break;
        }
    }

    // 0x004390D1
    static void on_mouse_down(ui::window* window, widget_index widgetIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case widx::tutorial_btn:
                sub_439112();
                break;
        }
    }

    // 0x004390DD
    static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case widx::tutorial_btn:
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

    static void sub_439163(ui::window* callingWindow, widget_index callingWidget)
    {
        windowmgr::close(window_type::multiplayer);

        static loco_global<char[16], 0x0112C826> commonFormatArgs;
        // (uint16_t)commonFormatArgs[8] = (uint16_t)string_ids::the_other_player;

        textinput::open_textinput(callingWindow, string_ids::chat_title, string_ids::chat_instructions, string_ids::null, callingWidget, commonFormatArgs);
    }

    static void sub_43918F(char string[512])
    {
        //        if (cl == 0)
        //        {
        //            return;
        //        }
        //
        //        0x9c68e8 = 0;
        //
        //        for (int i = 0; i < 32; i++)
        //        {
        //        }
    }

    static void sub_4391CC()
    {
        //        if (ax == -1)
        //        {
        //            return;
        //        }
        //
        //        registers regs;
        //        regs.ax = ax;
        //        call(0x43c590, regs); // tutorial::start();
    }

    static void sub_4391DA()
    {
        call(0x443868); // scenario_select::open()
    }

    static void sub_4391E2()
    {
        // do_game_command
        {
            registers regs;
            regs.bl = 1;
            regs.dl = 0;
            regs.di = 0;
            regs.esi = 21;
            call(0x00431315, regs);
        }
    }

    static void sub_46E328()
    {
        call(0x0046e328);
    }

    // 0x004391F9
    static void on_update(window* window)
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
