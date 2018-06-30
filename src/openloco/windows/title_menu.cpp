#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
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
    static const uint16_t btn_main_size = 74;
    static const uint16_t btn_sub_height = 18;
    static const uint16_t ww = btn_main_size * 4;
    static const uint16_t wh = btn_main_size + btn_sub_height;

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
    static ui::cursor_id on_cursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
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

        auto window = openloco::ui::windowmgr::create_window(
            window_type::title_menu,
            (ui::width() - ww) / 2,
            ui::height() - wh - 25,
            ww,
            wh,
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::scenario_list_btn) | (1 << widx::load_game_btn) | (1 << widx::tutorial_btn) | (1 << widx::scenario_editor_btn) | (1 << widx::chat_btn);

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

        // TODO: add widget::set_origin()
        window->widgets[widx::scenario_list_btn].left = 0;
        window->widgets[widx::scenario_list_btn].right = btn_main_size - 1;
        window->widgets[widx::load_game_btn].left = btn_main_size;
        window->widgets[widx::load_game_btn].right = btn_main_size * 2 - 1;
        window->widgets[widx::tutorial_btn].left = btn_main_size * 2;
        window->widgets[widx::tutorial_btn].right = btn_main_size * 3 - 1;
        window->widgets[widx::scenario_editor_btn].left = btn_main_size * 3;
        window->widgets[widx::scenario_editor_btn].right = btn_main_size * 4 - 1;
        window->widgets[widx::chat_btn].type = ui::widget_type::none;

        if (openloco::get_screen_flags() & screen_flags::unknown_2)
        {
            window->widgets[widx::tutorial_btn].type = ui::widget_type::none;
            window->widgets[widx::scenario_editor_btn].type = ui::widget_type::none;

            window->widgets[widx::scenario_list_btn].left = btn_main_size;
            window->widgets[widx::scenario_list_btn].right = btn_main_size * 2 - 1;
            window->widgets[widx::load_game_btn].left = btn_main_size * 2;
            window->widgets[widx::load_game_btn].right = btn_main_size * 3 - 1;

            window->widgets[widx::chat_btn].type = ui::widget_type::wt_9;
            interface_skin_object* skin = objectmgr::get<interface_skin_object>();
            window->widgets[widx::chat_btn].image = skin->img + interface_skin::image_ids::phone;
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

            uint32_t image_id = image_ids::title_menu_globe_spin_0;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::scenario_list_btn))
            {
                // TODO: add list of images
                image_id = image_ids::title_menu_globe_spin_0 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, image_ids::title_menu_sparkle);
        }

        if (window->widgets[widx::load_game_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::load_game_btn].left + window->x;
            int16_t y = window->widgets[widx::load_game_btn].top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_spin_0;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::load_game_btn))
            {
                // TODO: add list of images
                image_id = image_ids::title_menu_globe_spin_0 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
            openloco::gfx::draw_image(dpi, x, y, image_ids::title_menu_save);
        }

        if (window->widgets[widx::tutorial_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::tutorial_btn].left + window->x;
            int16_t y = window->widgets[widx::tutorial_btn].top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_spin_0;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::tutorial_btn))
            {
                // TODO: add list of images
                image_id = image_ids::title_menu_globe_spin_0 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);

            // TODO: base lesson overlay on language
            openloco::gfx::draw_image(dpi, x, y, image_ids::title_menu_lesson_l);
        }

        if (window->widgets[widx::scenario_editor_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::scenario_editor_btn].left + window->x;
            int16_t y = window->widgets[widx::scenario_editor_btn].top + window->y;

            uint32_t image_id = image_ids::title_menu_globe_construct_24;
            if (input::is_hovering(window_type::title_menu) && (input::get_hovered_widget_index() == widx::scenario_editor_btn))
            {
                // TODO: add list of images
                image_id = image_ids::title_menu_globe_construct_0 + ((window->var_846 / 2) % 32);
            }

            openloco::gfx::draw_image(dpi, x, y, image_id);
        }

        {
            int16_t y = window->widgets[widx::multiplayer_toggle_btn].top + 3 + window->y;
            int16_t x = window->width / 2 + window->x;

            string_id string = string_ids::single_player_mode;

            if ((openloco::get_screen_flags() & 1 << 2) != 0)
            {
                // char[512+1]
                auto buffer = stringmgr::get_string(string_ids::buffer_2039);

                char* playerName = (char*)0xF254D0;

                strcpy((char*)buffer, playerName);

                // common_format_args[0] = string_ids::buffer_2039;
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
    static ui::cursor_id on_cursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
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

    /**
     * 0x004CCA6D
     * x @<cx>
     * y @<dx>
     * width @<bp>
     * height @<di>
     * colour @<al>
     * count @<bl>
     * flags @<bh>
     */
    static void window_dropdown_show_text(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, int8_t flags)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bl = count;
        regs.bh = flags;
        regs.bp = width;
        regs.di = height;

        call(0x4CCA6D, regs);
    }

    static void sub_439112(window* window)
    {
        // dropdownFormat[0] = STR_1879
        // dropdownFormat[1] = STR_1880
        // dropdownFormat[2] = STR_1881

        widget_t* widget = &window->widgets[widx::tutorial_btn];

        window_dropdown_show_text(
            window->x + widget->left,
            window->y + widget->top,
            widget->width(),
            widget->height(),
            colour::translucent(window->colours[0]),
            3,
            0x8);
    }

    static void sub_439163(ui::window* callingWindow, widget_index callingWidget)
    {
        windowmgr::close(window_type::multiplayer);

        static loco_global<char[16], 0x0112C826> commonFormatArgs;
        // (uint16_t)commonFormatArgs[8] = (uint16_t)string_ids::the_other_player;

        // TODO: convert this to a builder pattern, with chainable functions to set the different string ids and arguments
        textinput::open_textinput(callingWindow, string_ids::chat_title, string_ids::chat_instructions, string_ids::empty, callingWidget, commonFormatArgs);
    }

    static void sub_43918F(char string[512])
    {
        addr<0x009C68E8, int16_t>() = 0;

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

        if ((addr<0x00508F10, uint16_t>() & 0x300) == 0)
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

        window->invalidate();
    }
}
