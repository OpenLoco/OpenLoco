#pragma once

#include "graphics/gfx.h"
#include "localisation/stringmgr.h"
#include "window.h"
#include <cstddef>

namespace openloco::ui
{
    enum class window_type : uint8_t
    {
        main = 0,
        toolbar_top = 1,
        toolbar_player_info = 2,
        toolbar_time = 3,

        tooltip = 6,
        dropdown = 7,

        about = 9,
        // The Atari credits window is no longer used
        about_atari = 10,
        about_music = 11,
        error = 12,
        construction = 13,
        prompt_save_game = 14,
        terraform = 15,
        title_menu = 16,
        title_exit = 17,
        scenario_select = 18,
        keyboard_shortcuts = 19,
        keyboard_shortcuts_edit = 20,
        map = 21,
        title_logo = 22,
        vehicle = 23,
        station = 24,

        company = 26,
        vehicle_list = 27,
        build_vehicle = 28,
        station_list = 29,

        object_selection = 31,
        town_list = 32,
        town = 33,
        industry = 34,
        industry_list = 35,
        unk_36 = 36,

        messages = 37,

        multiplayer = 39,
        options = 40,
        music_selection = 41,
        company_face_selection = 42,
        landscape_generation = 43,

        scenario_options = 45,

        wt_47 = 47,
        company_list = 48,
        tutorial = 49,
        prompt_confirm_display_mode = 50,
        text_input = 51,
        prompt_browse = 52,

        prompt_ok_cancel = 54,
        openloco_version = 55,
        title_options = 56,

        undefined = 255
    };
}

namespace openloco::ui::windowmgr
{
    void register_hooks();
    window_type current_modal_type();
    void current_modal_type(window_type type);
    window* get(size_t index);
    size_t num_windows();

    void update();
    window* get_main();
    window* find(window_type type);
    window* find(window_type type, window_number number);
    window* find_at(int16_t x, int16_t y);
    window* find_at_alt(int16_t x, int16_t y);
    window* bring_to_front(window* window);
    window* bring_to_front(window_type type, uint16_t id);
    void invalidate(window_type type);
    void invalidate(window_type type, window_number number);
    void invalidate_widget(window_type type, window_number number, uint8_t widget_index);
    void invalidate_all_windows_after_input();
    void close(window_type type);
    void close(window_type type, uint16_t id);
    void close(window* window);
    window* create_window(window_type type, int32_t x, int32_t y, int32_t width, int32_t height, int32_t flags, window_event_list* events);
    window* create_window_centred(window_type type, int32_t width, int32_t height, int32_t flags, window_event_list* events);
    void draw_single(gfx::drawpixelinfo_t* dpi, window* w, int32_t left, int32_t top, int32_t right, int32_t bottom);
    void dispatch_update_all();
    void call_event_viewport_rotate_on_all_windows();
    void relocate_windows();
    void sub_4CEE0B(window* self);
    void sub_4B93A5(window_number number);
    void close_topmost();
    void all_wheel_input();
}

namespace openloco::ui::windows
{

    void construction_mouse_up(window& w, uint16_t widgetIndex);
    void station_2_scroll_paint(window& w, gfx::drawpixelinfo_t& dpi);
    window* open_town_window(uint16_t townId);
    window* open_title_version();
    window* open_title_exit();
    window* open_title_menu();
    window* open_title_logo();
    void open_about_window();
    void sub_498E9B(window* w);

    bool prompt_ok_cancel(string_id okButtonStringId);
    void map_center_on_view_point();
}

namespace openloco::ui::about
{
    void open();
}

namespace openloco::ui::about_music
{
    void open();
}

namespace openloco::ui::options
{
    window* open();
    window* open_music_settings();
}

namespace openloco::ui::prompt_browse
{
    enum class browse_type
    {
        load = 1,
        save = 2
    };
    bool open(browse_type type, char* path, const char* filter, const char* title);
    void register_hooks();
}

namespace openloco::ui::textinput
{
    void register_hooks();

    void open_textinput(ui::window* w, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs);
    void sub_4CE6C9(window_type type, window_number number);
    void cancel();
    void sub_4CE910(int eax, int ebx);
    void sub_4CE6FF();
}

namespace openloco::ui::title_options
{
    window* open();
}

namespace openloco::ui::tooltip
{
    void register_hooks();
    void open(ui::window* window, int32_t widgetIndex, int16_t x, int16_t y);
    void update(ui::window* window, int32_t widgetIndex, string_id stringId, int16_t x, int16_t y);
}
