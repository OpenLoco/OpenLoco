#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::music_selection
{
    static const gfx::ui_size_t window_size = { 360, 238 };

    static const uint8_t rowHeight = 12; // CJK: 15
    static const uint8_t numTracks = 29;

    enum widx
    {
        frame,
        title,
        close,
        panel,
        scrollview,
    };

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 360, 238 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 358, 13 }, widget_type::caption_25, 0, string_ids::music_selection_title),
        make_widget({ 345, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 360, 223 }, widget_type::panel, 1),
        make_widget({ 4, 19 }, { 352, 218 }, widget_type::scrollview, 1, scrollbars::vertical, string_ids::music_selection_tooltip),
        widget_end(),
    };

    static window_event_list _events;

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_update(window* window);
    static void tooltip(ui::window* window, widget_index widgetIndex);

    static void init_events()
    {
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;
        _events.get_scroll_size = get_scroll_size;
        _events.on_mouse_up = on_mouse_up;
        _events.on_update = on_update;
        _events.scroll_mouse_down = on_scroll_mouse_down;
        _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.tooltip = tooltip;
    }

    // 0x004C1602
    window* open()
    {
        window* window = WindowManager::bringToFront(WindowType::musicSelection, 0);
        if (window != nullptr)
            return window;

        window = WindowManager::createWindow(
            WindowType::musicSelection,
            window_size,
            0,
            &_events);

        // TODO: only needs to be called once.
        init_events();

        window->widgets = _widgets;
        window->enabled_widgets = 1 << widx::close;
        window->init_scroll_widgets();

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        window->row_count = numTracks;
        window->row_hover = -1;

        return window;
    }

    // 0x004C165D
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);
    }

    // 0x004C1663
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        auto shade = colour::get_shade(window->colours[1], 4);
        gfx::clear_single(*dpi, shade);

        static const string_id music_ids_to_string_id[] = {
            string_ids::music_chuggin_along,
            string_ids::music_long_dusty_road,
            string_ids::music_flying_high,
            string_ids::music_gettin_on_the_gas,
            string_ids::music_jumpin_the_rails,
            string_ids::music_smooth_running,
            string_ids::music_traffic_jam,
            string_ids::music_never_stop_til_you_get_there,
            string_ids::music_soaring_away,
            string_ids::music_techno_torture,
            string_ids::music_everlasting_high_rise,
            string_ids::music_solace,
            string_ids::music_chrysanthemum,
            string_ids::music_eugenia,
            string_ids::music_the_ragtime_dance,
            string_ids::music_easy_winners,
            string_ids::music_setting_off,
            string_ids::music_a_travellers_seranade,
            string_ids::music_latino_trip,
            string_ids::music_a_good_head_of_steam,
            string_ids::music_hop_to_the_bop,
            string_ids::music_the_city_lights,
            string_ids::music_steamin_down_town,
            string_ids::music_bright_expectations,
            string_ids::music_mo_station,
            string_ids::music_far_out,
            string_ids::music_running_on_time,
            string_ids::music_get_me_to_gladstone_bay,
            string_ids::music_sandy_track_blues,
        };

        auto config = config::get();

        uint16_t y = 0;
        for (uint16_t i = 0; i < window->row_count; i++)
        {
            string_id text_colour_id = string_ids::white_stringid2;

            // Draw hovered track
            if (i == window->row_hover)
            {
                gfx::draw_rect(dpi, 0, y, 800, rowHeight, 0x2000030);
                text_colour_id = string_ids::wcolour2_stringid2;
            }

            // Draw checkbox.
            gfx::fill_rect_inset(dpi, 2, y, 11, y + 10, window->colours[1], 0xE0);

            // Draw checkmark if track is enabled.
            if (config.enabled_music[i])
                gfx::draw_string_494B3F(*dpi, 2, y, window->colours[1], string_ids::wcolour2_stringid2, (void*)&string_ids::checkmark);

            // Draw track name.
            gfx::draw_string_494B3F(*dpi, 15, y, window->colours[1], text_colour_id, (void*)&music_ids_to_string_id[i]);

            y += rowHeight;
        }
    }

    // 0x004C176C
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = rowHeight * numTracks;
    }

    // 0x004C1757
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window->type);
                break;
        }
    }

    // 0x004C1799
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentTrack = y / rowHeight;
        if (currentTrack > window->row_count)
            return;

        auto& config = config::get();

        // Toggle the track in question.
        config.enabled_music[currentTrack] = !config.enabled_music[currentTrack];

        // Are any tracks enabled?
        uint8_t anyEnabled = 0;
        for (uint8_t i = 0; i < numTracks; i++)
            anyEnabled |= config.enabled_music[i];

        // Ensure at least this track is enabled.
        if (!anyEnabled)
            config.enabled_music[currentTrack] = true;

        config::write();
        call(0x048AA0C);
        window->invalidate();
    }

    // 0x004C1771
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentTrack = y / rowHeight;
        if (currentTrack > window->row_count || currentTrack == window->row_hover)
            return;

        window->row_hover = currentTrack;
        window->invalidate();
    }

    // 0x004C17E3
    static void on_update(window* window)
    {
        auto optionsWindow = WindowManager::find(WindowType::options);
        if (optionsWindow == nullptr || optionsWindow->current_tab != options::tab_offset_music)
        {
            WindowManager::close(window);
            return;
        }
    }

    // 0x004C1762
    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = string_ids::tooltip_scroll_list;
    }
}
