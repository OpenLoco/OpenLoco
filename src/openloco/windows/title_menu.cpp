#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../multiplayer.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static const uint8_t btn_main_size = 74;
    static const uint8_t btn_sub_height = 18;
    static const uint16_t ww = btn_main_size * 4;
    static const uint16_t wh = btn_main_size + btn_sub_height;

    static const std::vector<uint32_t> globe_spin = {
        image_ids::title_menu_globe_spin_0,
        image_ids::title_menu_globe_spin_1,
        image_ids::title_menu_globe_spin_2,
        image_ids::title_menu_globe_spin_3,
        image_ids::title_menu_globe_spin_4,
        image_ids::title_menu_globe_spin_5,
        image_ids::title_menu_globe_spin_6,
        image_ids::title_menu_globe_spin_7,
        image_ids::title_menu_globe_spin_8,
        image_ids::title_menu_globe_spin_9,
        image_ids::title_menu_globe_spin_10,
        image_ids::title_menu_globe_spin_11,
        image_ids::title_menu_globe_spin_12,
        image_ids::title_menu_globe_spin_13,
        image_ids::title_menu_globe_spin_14,
        image_ids::title_menu_globe_spin_15,
        image_ids::title_menu_globe_spin_16,
        image_ids::title_menu_globe_spin_17,
        image_ids::title_menu_globe_spin_18,
        image_ids::title_menu_globe_spin_19,
        image_ids::title_menu_globe_spin_20,
        image_ids::title_menu_globe_spin_21,
        image_ids::title_menu_globe_spin_22,
        image_ids::title_menu_globe_spin_23,
        image_ids::title_menu_globe_spin_24,
        image_ids::title_menu_globe_spin_25,
        image_ids::title_menu_globe_spin_26,
        image_ids::title_menu_globe_spin_27,
        image_ids::title_menu_globe_spin_28,
        image_ids::title_menu_globe_spin_29,
        image_ids::title_menu_globe_spin_30,
        image_ids::title_menu_globe_spin_31,
    };

    static const std::vector<uint32_t> globe_construct = {
        image_ids::title_menu_globe_construct_0,
        image_ids::title_menu_globe_construct_1,
        image_ids::title_menu_globe_construct_2,
        image_ids::title_menu_globe_construct_3,
        image_ids::title_menu_globe_construct_4,
        image_ids::title_menu_globe_construct_5,
        image_ids::title_menu_globe_construct_6,
        image_ids::title_menu_globe_construct_7,
        image_ids::title_menu_globe_construct_8,
        image_ids::title_menu_globe_construct_9,
        image_ids::title_menu_globe_construct_10,
        image_ids::title_menu_globe_construct_11,
        image_ids::title_menu_globe_construct_12,
        image_ids::title_menu_globe_construct_13,
        image_ids::title_menu_globe_construct_14,
        image_ids::title_menu_globe_construct_15,
        image_ids::title_menu_globe_construct_16,
        image_ids::title_menu_globe_construct_17,
        image_ids::title_menu_globe_construct_18,
        image_ids::title_menu_globe_construct_19,
        image_ids::title_menu_globe_construct_20,
        image_ids::title_menu_globe_construct_21,
        image_ids::title_menu_globe_construct_22,
        image_ids::title_menu_globe_construct_23,
        image_ids::title_menu_globe_construct_24,
        image_ids::title_menu_globe_construct_25,
        image_ids::title_menu_globe_construct_26,
        image_ids::title_menu_globe_construct_27,
        image_ids::title_menu_globe_construct_28,
        image_ids::title_menu_globe_construct_29,
        image_ids::title_menu_globe_construct_30,
        image_ids::title_menu_globe_construct_31,
    };

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
        make_widget({ 0, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_new_game),
        make_widget({ btn_main_size, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_load_game),
        make_widget({ btn_main_size * 2, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_show_tutorial),
        make_widget({ btn_main_size * 3, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_scenario_editor),
        make_widget({ btn_main_size * 4 - 31, btn_main_size - 27 }, { 31, 27 }, widget_type::wt_9, 1, string_ids::null, string_ids::title_menu_chat_tooltip),
        make_widget({ 0, btn_main_size }, { ww, btn_sub_height }, widget_type::wt_9, 1, string_ids::null, string_ids::title_multiplayer_toggle_tooltip),
        widget_end(),
    };

    static window_event_list _events;

    static void sub_439112(window* window);
    static void sub_4391CC(int16_t itemIndex);
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
    static void on_text_input(window* window, widget_index widgetIndex, char* input);
    static ui::cursor_id on_cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void prepare_draw(ui::window* window);

    // static loco_global<window_event_list[1], 0x004f9ec8> _events;

    ui::window* open_title_menu()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.text_input = on_text_input;
        _events.cursor = on_cursor;
        _events.on_update = on_update;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;

        auto window = openloco::ui::WindowManager::createWindow(
            WindowType::titleMenu,
            gfx::point_t((ui::width() - ww) / 2, ui::height() - wh - 25),
            { ww, wh },
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = (loco_ptr)_widgets;
        window->enabled_widgets = (1 << widx::scenario_list_btn) | (1 << widx::load_game_btn) | (1 << widx::tutorial_btn) | (1 << widx::scenario_editor_btn) | (1 << widx::chat_btn) | (1 << widx::multiplayer_toggle_btn);

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
        window->getWidget(widx::tutorial_btn)->type = ui::widget_type::wt_9;
        window->getWidget(widx::scenario_editor_btn)->type = ui::widget_type::wt_9;

        // TODO: add widget::set_origin()
        window->getWidget(widx::scenario_list_btn)->left = 0;
        window->getWidget(widx::scenario_list_btn)->right = btn_main_size - 1;
        window->getWidget(widx::load_game_btn)->left = btn_main_size;
        window->getWidget(widx::load_game_btn)->right = btn_main_size * 2 - 1;
        window->getWidget(widx::tutorial_btn)->left = btn_main_size * 2;
        window->getWidget(widx::tutorial_btn)->right = btn_main_size * 3 - 1;
        window->getWidget(widx::scenario_editor_btn)->left = btn_main_size * 3;
        window->getWidget(widx::scenario_editor_btn)->right = btn_main_size * 4 - 1;
        window->getWidget(widx::chat_btn)->type = ui::widget_type::none;

        if (openloco::isNetworked())
        {
            window->getWidget(widx::tutorial_btn)->type = ui::widget_type::none;
            window->getWidget(widx::scenario_editor_btn)->type = ui::widget_type::none;

            window->getWidget(widx::scenario_list_btn)->left = btn_main_size;
            window->getWidget(widx::scenario_list_btn)->right = btn_main_size * 2 - 1;
            window->getWidget(widx::load_game_btn)->left = btn_main_size * 2;
            window->getWidget(widx::load_game_btn)->right = btn_main_size * 3 - 1;

            window->getWidget(widx::chat_btn)->type = ui::widget_type::wt_9;
            interface_skin_object* skin = objectmgr::get<interface_skin_object>();
            window->getWidget(widx::chat_btn)->image = skin->img + interface_skin::image_ids::phone;
        }
    }

    // 0x00438EC7
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        if (window->getWidget(widx::scenario_list_btn)->type != ui::widget_type::none)
        {
            int16_t x = window->getWidget(widx::scenario_list_btn)->left + window->x;
            int16_t y = window->getWidget(widx::scenario_list_btn)->top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_spin_0;
            if (input::is_hovering(WindowType::titleMenu) && (input::get_hovered_widget_index() == widx::scenario_list_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, image_ids::title_menu_sparkle);
        }

        if (window->getWidget(widx::load_game_btn)->type != ui::widget_type::none)
        {
            int16_t x = window->getWidget(widx::load_game_btn)->left + window->x;
            int16_t y = window->getWidget(widx::load_game_btn)->top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_spin_0;
            if (input::is_hovering(WindowType::titleMenu) && (input::get_hovered_widget_index() == widx::load_game_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, image_ids::title_menu_save);
        }

        if (window->getWidget(widx::tutorial_btn)->type != ui::widget_type::none)
        {
            int16_t x = window->getWidget(widx::tutorial_btn)->left + window->x;
            int16_t y = window->getWidget(widx::tutorial_btn)->top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_spin_0;
            if (input::is_hovering(WindowType::titleMenu) && (input::get_hovered_widget_index() == widx::tutorial_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);

            // TODO: base lesson overlay on language
            openloco::gfx::draw_image(dpi, x, y, image_ids::title_menu_lesson_l);
        }

        if (window->getWidget(widx::scenario_editor_btn)->type != ui::widget_type::none)
        {
            int16_t x = window->getWidget(widx::scenario_editor_btn)->left + window->x;
            int16_t y = window->getWidget(widx::scenario_editor_btn)->top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_construct_24;
            if (input::is_hovering(WindowType::titleMenu) && (input::get_hovered_widget_index() == widx::scenario_editor_btn))
            {
                image_id = globe_construct[((window->var_846 / 2) % globe_construct.size())];
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
        }

        {
            int16_t y = window->getWidget(widx::multiplayer_toggle_btn)->top + 3 + window->y;
            int16_t x = window->width / 2 + window->x;

            string_id string = string_ids::single_player_mode;

            if (openloco::isNetworked())
            {
                // char[512+1]
                auto buffer = stringmgr::get_string(string_ids::buffer_2039);

                char* playerName = (char*)0xF254D0;

                strcpy((char*)buffer, playerName);

                addr<0x112C826, string_id>() = string_ids::buffer_2039;
                string = string_ids::two_player_mode_connected;
            }

            draw_string_centred_clipped(*dpi, x, y, ww - 4, colour::black, string, (char*)0x112c826);
        }
    }

    // 0x00439094
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        if (intro::is_active())
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
                sub_439112(window);
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
                sub_4391CC(itemIndex);
                break;
        }
    }

    // 0x004390ED
    static void on_text_input(window* window, widget_index widgetIndex, char* input)
    {
        switch (widgetIndex)
        {
            case widx::chat_btn:
                sub_43918F(input);
                break;
        }
    }

    // 0x004390f8
    static ui::cursor_id on_cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        // Reset tooltip timeout to keep tooltips open.
        addr<0x0052338A, uint16_t>() = 2000;
        return fallback;
    }

    static void sub_439102()
    {
        call(0x0046e639); // window_multiplayer::open
    }

    static void sub_43910A()
    {
        call(0x0043D7DC); // show_scenario_editor
    }

    static void sub_439112(window* window)
    {
        dropdown::add(0, string_ids::tutorial_1);
        dropdown::add(1, string_ids::tutorial_2);
        dropdown::add(2, string_ids::tutorial_3);

        widget_t* widget = window->getWidget(widx::tutorial_btn);
        dropdown::show_text(
            window->x + widget->left,
            window->y + widget->top,
            widget->width(),
            widget->height(),
            colour::translucent(window->colours[0]),
            3,
            0x80);
    }

    static void sub_439163(ui::window* callingWindow, widget_index callingWidget)
    {
        WindowManager::close(WindowType::multiplayer);

        addr<0x112C826 + 8, string_id>() = string_ids::the_other_player;

        // TODO: convert this to a builder pattern, with chainable functions to set the different string ids and arguments
        textinput::open_textinput(callingWindow, string_ids::chat_title, string_ids::chat_instructions, string_ids::empty, callingWidget, (void*)0x112C826);
    }

    static void sub_43918F(char string[512])
    {
        addr<0x009C68E8, string_id>() = string_ids::empty;

        for (int i = 0; i < 32; i++)
        {
            game_commands::do_71(i, &string[i * 16]);
        }
    }

    static void sub_4391CC(int16_t itemIndex)
    {
        // DROPDOWN_ITEM_UNDEFINED
        if (itemIndex == -1)
            return;

        registers regs;
        regs.ax = itemIndex;
        call(0x43c590, regs); // tutorial::start();
    }

    static void sub_4391DA()
    {
        call(0x443868); // scenario_select::open()
    }

    static void sub_4391E2()
    {
        game_commands::do_21(1, 0, 0);
    }

    static void sub_46E328()
    {
        call(0x0046e328);
    }

    // 0x004391F9
    static void on_update(window* window)
    {
        window->var_846++;

        if (intro::is_active())
        {
            window->invalidate();
            return;
        }

        if (!multiplayer::has_flag(multiplayer::flags::flag_8) && !multiplayer::has_flag(multiplayer::flags::flag_9))
        {
            window->invalidate();
            return;
        }

        if (addr<0x0050C1AE, int32_t>() < 20)
        {
            window->invalidate();
            return;
        }

        auto multiplayer = WindowManager::find(WindowType::multiplayer);
        if (multiplayer == nullptr)
        {
            call(0x0046e639); // window_multiplayer::open
        }

        window->invalidate();
    }
}
