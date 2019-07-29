#include "../companymgr.h"
#include "../date.h"
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
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"

using namespace openloco::interop;

namespace openloco::ui::TimePanel
{
    static const gfx::ui_size_t window_size = { 140, 27 };

    namespace widx
    {
        enum
        {
            outer_frame,
            inner_frame,
            map_chat_menu,
            date_btn,
            pause_btn,
            normal_speed_btn,
            fast_forward_btn,
            extra_fast_forward_btn,
        };
    }

    static void formatChallenge();
    static void processChatMessage(char* str);
    static void togglePaused();
    static void changeGameSpeed(window* w, uint8_t speed);

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 140, 29 }, widget_type::wt_3, 0),                                                                                           // 0,
        make_widget({ 2, 2 }, { 136, 25 }, widget_type::wt_3, 0),                                                                                           // 1,
        make_widget({ 113, 1 }, { 26, 26 }, widget_type::wt_9, 0),                                                                                          // 2,
        make_widget({ 2, 2 }, { 111, 12 }, widget_type::wt_9, 0, image_ids::null, string_ids::tooltip_daymonthyear_challenge),                              // 3,
        make_remap_widget({ 18, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_pause, string_ids::tooltip_speed_pause),                           // 4,
        make_remap_widget({ 38, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_normal, string_ids::tooltip_speed_normal),                         // 5,
        make_remap_widget({ 58, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_fast_forward, string_ids::tooltip_speed_fast_forward),             // 6,
        make_remap_widget({ 78, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_extra_fast_forward, string_ids::tooltip_speed_extra_fast_forward), // 7,
        widget_end(),
    };

    static window_event_list _events;

    static void prepare_draw(window* window);
    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_mouse_down(ui::window* window, widget_index widgetIndex);
    static void text_input(window* w, widget_index widgetIndex, char* str);
    static void on_dropdown(window* w, widget_index widgetIndex, int16_t item_index);
    static ui::cursor_id on_cursor(window* w, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void tooltip(ui::window* window, widget_index widgetIndex);
    static void text_input(window* w, widget_index widgetIndex, char* str);
    static void on_update(window* w);

    static loco_global<uint16_t, 0x0050A004> _50A004;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;
    static loco_global<uint8_t, 0x00508F1A> game_speed;

    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526243> _526243;

    loco_global<uint16_t[8], 0x112C826> _common_format_args;

    window* open()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.event_03 = on_mouse_down;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = on_update;
        _events.text_input = text_input;
        _events.tooltip = tooltip;
        _events.cursor = on_cursor;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::timeToolbar,
            gfx::point_t(ui::width() - window_size.width, ui::height() - window_size.height),
            gfx::ui_size_t(window_size.width, window_size.height),
            ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::map_chat_menu) | (1 << widx::date_btn) | (1 << widx::pause_btn) | (1 << widx::normal_speed_btn) | (1 << widx::fast_forward_btn) | (1 << widx::extra_fast_forward_btn);
        window->var_854 = 0;
        window->var_856 = 0;
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = colour::translucent(skin->colour_17);
            window->colours[1] = colour::translucent(skin->colour_17);
        }

        return window;
    }

    // 0x004396A4
    static void prepare_draw(window* window)
    {
        _widgets[widx::inner_frame].type = widget_type::none;
        _widgets[widx::pause_btn].image = gfx::recolour(image_ids::speed_pause);
        _widgets[widx::normal_speed_btn].image = gfx::recolour(image_ids::speed_normal);
        _widgets[widx::fast_forward_btn].image = gfx::recolour(image_ids::speed_fast_forward);
        _widgets[widx::extra_fast_forward_btn].image = gfx::recolour(image_ids::speed_extra_fast_forward);

        if (is_paused())
        {
            _widgets[widx::pause_btn].image = gfx::recolour(image_ids::speed_pause_active);
        }
        else if (game_speed == 0)
        {
            _widgets[widx::normal_speed_btn].image = gfx::recolour(image_ids::speed_normal_active);
        }
        else if (game_speed == 1)
        {
            _widgets[widx::fast_forward_btn].image = gfx::recolour(image_ids::speed_fast_forward_active);
        }
        else if (game_speed == 2)
        {
            _widgets[widx::extra_fast_forward_btn].image = gfx::recolour(image_ids::speed_extra_fast_forward_active);
        }

        if (isNetworked())
        {
            _widgets[widx::fast_forward_btn].type = widget_type::none;
            _widgets[widx::extra_fast_forward_btn].type = widget_type::none;

            _widgets[widx::pause_btn].left = 38;
            _widgets[widx::pause_btn].right = 57;
            _widgets[widx::normal_speed_btn].left = 58;
            _widgets[widx::normal_speed_btn].right = 77;
        }
        else
        {
            _widgets[widx::fast_forward_btn].type = widget_type::wt_9;
            _widgets[widx::extra_fast_forward_btn].type = widget_type::wt_9;

            _widgets[widx::pause_btn].left = 18;
            _widgets[widx::pause_btn].right = 37;
            _widgets[widx::normal_speed_btn].left = 38;
            _widgets[widx::normal_speed_btn].right = 57;
            _widgets[widx::fast_forward_btn].left = 58;
            _widgets[widx::fast_forward_btn].right = 77;
            _widgets[widx::extra_fast_forward_btn].left = 78;
            _widgets[widx::extra_fast_forward_btn].right = 97;
        }
    }

    // TODO: use same list as top toolbar
    static const uint32_t map_sprites_by_rotation[] = {
        interface_skin::image_ids::toolbar_menu_map_north,
        interface_skin::image_ids::toolbar_menu_map_west,
        interface_skin::image_ids::toolbar_menu_map_south,
        interface_skin::image_ids::toolbar_menu_map_east,
    };

    // 0x004397BE
    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
    {
        widget_t& frame = _widgets[widx::outer_frame];
        gfx::draw_rect(dpi, self->x + frame.left, self->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        self->draw(dpi);

        gfx::draw_rect_inset(dpi, self->x + frame.left + 1, self->y + frame.top + 1, frame.width() - 2, frame.height() - 2, self->colours[1], 0x30);

        *(uint32_t*)&_common_format_args[0] = current_day();
        string_id format = string_ids::date_monthyear;

        if (is_paused() && (get_pause_flags() & (1 << 2)) == 0)
        {
            if (self->var_856 >= 30)
            {
                format = string_ids::toolbar_status_paused;
            }
        }

        colour_t c = colour::opaque(self->colours[0]);
        if (input::is_hovering(WindowType::timeToolbar) && (input::get_hovered_widget_index() == widx::date_btn))
        {
            c = colour::white;
        }
        gfx::draw_string_centred(*dpi, self->x + _widgets[widx::date_btn].mid_x(), self->y + _widgets[widx::date_btn].top + 1, c, format, &*_common_format_args);

        auto skin = objectmgr::get<interface_skin_object>();
        gfx::draw_image(dpi, self->x + _widgets[widx::map_chat_menu].left - 2, self->y + _widgets[widx::map_chat_menu].top - 1, skin->img + map_sprites_by_rotation[gCurrentRotation]);
    }

    // 0x004398FB
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::date_btn:
                MessageWindow::open();
                break;
            case widx::pause_btn:
                togglePaused();
                break;
            case widx::normal_speed_btn:
                changeGameSpeed(window, 0);
                break;
            case widx::fast_forward_btn:
                changeGameSpeed(window, 1);
                break;
            case widx::extra_fast_forward_btn:
                changeGameSpeed(window, 2);
                break;
        }
    }

    // 0x0043A67F
    static void map_mouse_down(ui::window* self, widget_index widgetIndex)
    {
        auto skin = objectmgr::get<interface_skin_object>();

        if (isNetworked())
        {
            dropdown::add(0, string_ids::menu_sprite_stringid, { (uint32_t)skin->img + interface_skin::image_ids::phone, string_ids::chat_send_message });
            dropdown::add(1, string_ids::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[gCurrentRotation], string_ids::menu_map });
            dropdown::show_below(self, widgetIndex, 2, 25);
            dropdown::set_highlighted_item(1);
        }
        else
        {
            dropdown::add(0, string_ids::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[gCurrentRotation], string_ids::menu_map });
            dropdown::show_below(self, widgetIndex, 1, 25);
            dropdown::set_highlighted_item(0);
        }
    }

    // 0x0043A72F
    static void map_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        if (isNetworked())
        {
            switch (itemIndex)
            {
                case 0:
                {
                    auto opponent = companymgr::getOpponent();
                    _common_format_args[4] = opponent->var_02;
                    ui::textinput::open_textinput(self, string_ids::chat_title, string_ids::chat_instructions, string_ids::empty, widgetIndex, &*_common_format_args);
                    break;
                }
                case 1:
                    windows::map::open();
                    break;
            }
        }
        else
        {
            switch (itemIndex)
            {
                case 0:
                    windows::map::open();
                    break;
            }
        }
    }

    // 0x043992E
    static void on_mouse_down(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::map_chat_menu:
                map_mouse_down(window, widgetIndex);
                break;
        }
    }

    // 0x439939
    static void on_dropdown(window* w, widget_index widgetIndex, int16_t item_index)
    {
        switch (widgetIndex)
        {
            case widx::map_chat_menu:
                map_dropdown(w, widgetIndex, item_index);
                break;
        }
    }

    // 0x00439944
    static ui::cursor_id on_cursor(ui::window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        switch (widgetIdx)
        {
            case widx::date_btn:
                _tooltipTimeout = 2000;
                break;
        }

        return fallback;
    }

    // 0x00439955
    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::date_btn:
                formatChallenge();
                break;
        }
    }

    // 0x0043995C
    void formatChallenge()
    {
        _common_format_args[0] = current_day();

        auto playerCompany = companymgr::get(companymgr::get_controlling_id());

        if ((playerCompany->challenge_flags & company_flags::challenge_completed) != 0)
        {
            _common_format_args[2] = string_ids::challenge_completed;
        }
        else if ((playerCompany->challenge_flags & company_flags::challenge_failed) != 0)
        {
            _common_format_args[2] = string_ids::challenge_failed;
        }
        else if ((playerCompany->challenge_flags & company_flags::challenge_beaten_by_opponent) != 0)
        {
            _common_format_args[2] = string_ids::empty;
        }
        else
        {
            _common_format_args[2] = string_ids::challenge_progress;
            _common_format_args[3] = playerCompany->var_8C4E;

            if (objectiveFlags & 4)
            {
                auto monthsLeft = (*objectiveTimeLimitYears * 12 - _526243);
                auto yearsLeft = monthsLeft / 12;
                monthsLeft = monthsLeft % 12;
                _common_format_args[4] = string_ids::challenge_time_left;
                _common_format_args[5] = yearsLeft;
                _common_format_args[6] = monthsLeft;
            }
            else
            {
                _common_format_args[4] = string_ids::empty;
            }
        }
    }

    // 0x00439A15
    static void text_input(window* w, widget_index widgetIndex, char* str)
    {
        switch (widgetIndex)
        {
            case widx::map_chat_menu:
                processChatMessage(str);
                break;
        }
    }

    // 0x00439A1C
    static void processChatMessage(char string[512])
    {
        addr<0x009C68E8, string_id>() = string_ids::empty;

        for (uint8_t i = 0; i < 32; i++)
        {
            game_commands::do_71(i, &string[i * 16]);
        }
    }

    static void togglePaused()
    {
        game_commands::do_20();
    }

    // 0x00439A70 (speed: 0)
    // 0x00439A93 (speed: 1)
    // 0x00439AB6 (speed: 2)
    static void changeGameSpeed(window* w, uint8_t speed)
    {
        if (get_pause_flags() & 1)
        {
            game_commands::do_20();
        }

        game_speed = speed;
        w->invalidate();
    }

    // 0x00439AD9
    static void on_update(window* w)
    {
        w->var_854 += 1;
        if (w->var_854 >= 24)
        {
            w->var_854 = 0;
        }

        w->var_856 += 1;
        if (w->var_856 >= 60)
        {
            w->var_856 = 0;
        }

        if (_50A004 & (1 << 1))
        {
            _50A004 = _50A004 & ~(1 << 1);
            WindowManager::invalidateWidget(WindowType::timeToolbar, 0, widx::inner_frame);
        }

        if (is_paused())
        {
            WindowManager::invalidateWidget(WindowType::timeToolbar, 0, widx::inner_frame);
        }
    }
}
