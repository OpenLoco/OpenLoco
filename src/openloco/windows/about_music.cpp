#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::about_music
{
    constexpr uint16_t ww = 500;
    constexpr uint16_t wh = 312;

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            scrollview,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { ww, wh }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { ww - 2, 13 }, widget_type::caption_25, 0, string_ids::music_acknowledgements_caption),
        make_widget({ ww - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, 2321, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { ww, 297 }, widget_type::panel, 1),
        make_widget({ 4, 18 }, { ww - 8, 289 }, widget_type::scrollview, 1, 2),
        widget_end(),
    };

    static window_event_list _events;

    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void tooltip(ui::window* window, widget_index widgetIndex);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);

    // 0x0043B26C
    void open()
    {
        if (windowmgr::bring_to_front(window_type::about_music, 0) != nullptr)
            return;

        _events.on_mouse_up = on_mouse_up;
        _events.get_scroll_size = get_scroll_size;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;

        auto window = windowmgr::create_window_centred(
            window_type::about_music,
            ww,
            wh,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = 1 << widx::close;
        window->init_scroll_widgets();

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;
    }

    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                windowmgr::close(window->type);
                break;
        }
    }

    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = 1054;
    }

    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = string_ids::tooltip_scroll_credits_list;
    }

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);
    }

    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        static string_id strings_to_draw[] = {
            string_ids::locomotion_title,
            string_ids::locomotion_title_credit,
            string_ids::long_dusty_road,
            string_ids::long_dusty_road_credit,
            string_ids::flying_high,
            string_ids::flying_high_credit,
            string_ids::gettin_on_the_gas,
            string_ids::gettin_on_the_gas_credit,
            string_ids::jumpin_the_rails,
            string_ids::jumpin_the_rails_credit,
            string_ids::smooth_running,
            string_ids::smooth_running_credit,
            string_ids::traffic_jam,
            string_ids::traffic_jam_credit,
            string_ids::never_stop_til_you_get_there,
            string_ids::never_stop_til_you_get_there_credit,
            string_ids::soaring_away,
            string_ids::soaring_away_credit,
            string_ids::techno_torture,
            string_ids::techno_torture_credit,
            string_ids::everlasting_high_rise,
            string_ids::everlasting_high_rise_credit,
            string_ids::solace,
            string_ids::solace_credit,
            string_ids::chrysanthemum,
            string_ids::chrysanthemum_credit,
            string_ids::eugenia,
            string_ids::eugenia_credit,
            string_ids::the_ragtime_dance,
            string_ids::the_ragtime_dance_credit,
            string_ids::easy_winners,
            string_ids::easy_winners_credit,
            string_ids::setting_off,
            string_ids::setting_off_credit,
            string_ids::a_travellers_seranade,
            string_ids::a_travellers_seranade_credit,
            string_ids::latino_trip,
            string_ids::latino_trip_credit,
            string_ids::a_good_head_of_steam,
            string_ids::a_good_head_of_steam_credit,
            string_ids::hop_to_the_bop,
            string_ids::hop_to_the_bop_credit,
            string_ids::the_city_lights,
            string_ids::the_city_lights_credit,
            string_ids::steamin_down_town,
            string_ids::steamin_down_town_credit,
            string_ids::bright_expectations,
            string_ids::bright_expectations_credit,
            string_ids::mo_station,
            string_ids::mo_station_credit,
            string_ids::far_out,
            string_ids::far_out_credit,
            string_ids::running_on_time,
            string_ids::running_on_time_credit,
            string_ids::get_me_to_gladstone_bay,
            string_ids::get_me_to_gladstone_bay_credit,
            string_ids::chuggin_along,
            string_ids::chuggin_along_credit,
            string_ids::dont_lose_your_rag,
            string_ids::dont_lose_your_rag_credit,
            string_ids::sandy_track_blues,
            string_ids::sandy_track_blues_credit,
        };

        int16_t x = 240;
        int16_t y = 2;

        size_t i = 0;
        for (string_id credit_string : strings_to_draw)
        {
            // TODO: optimisation: don't draw past fold.

            draw_string_centred(*dpi, x, y, colour::black, credit_string, nullptr);
            y += 10;
            i++;

            // Show CS' copyright after every two lines.
            if (i % 2 == 0)
            {
                draw_string_centred(*dpi, x, y, colour::black, string_ids::music_copyright, nullptr);
                y += 14;
            }
        }
    }
}
