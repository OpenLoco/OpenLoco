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
        about = 9,
        about_atari = 10,
        about_music = 11,
        wt_12 = 12,
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
        undefined = 255
    };
}

namespace openloco::ui::windowmgr
{
    window_type current_modal_type();
    void current_modal_type(window_type type);
    window* get(size_t index);
    size_t num_windows();

    void update();
    window* get_main();
    window* find(window_type type);
    window* find(window_type type, uint16_t id);
    window* find_at(int16_t x, int16_t y);
    void invalidate(window_type type);
    void invalidate(window_type type, uint16_t id);
    void close(window_type type);
    void close(window_type type, uint16_t id);
    window* create_window(window_type type, int32_t x, int32_t y, int32_t width, int32_t height, int32_t flags, void* events);
    window* create_window_centred(window_type type, int32_t width, int32_t height, int32_t flags, void* events);
    void init_scroll_widgets(window* window);
    void draw_single(gfx::drawpixelinfo_t* dpi, window* w, int32_t left, int32_t top, int32_t right, int32_t bottom);
}

namespace openloco::ui::windows
{
    enum class browse_type
    {
        load = 1,
        save = 2
    };

    void construction_mouse_up(window& w, uint16_t widgetIndex);
    void station_2_scroll_paint(window& w, gfx::drawpixelinfo_t& dpi);
    window* open_town_window(uint16_t townId);
    window* open_title_version();
    void sub_498E9B(window* w);

    bool prompt_browse(browse_type type, char* path, const char* filter, const char* title);
    bool prompt_ok_cancel(string_id okButtonStringId);
}

namespace openloco::ui::textinput
{
    void close();
}

namespace openloco::ui::tooltip
{
    void register_hooks();
    void open(ui::window* window, int32_t widgetIndex, int16_t x, int16_t y);
    void update(ui::window* window, int32_t widgetIndex, int16_t x, int16_t y);
}
