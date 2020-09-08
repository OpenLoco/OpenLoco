#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/interface_skin_object.h"
#include "../Objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::AboutMusic
{
    constexpr gfx::ui_size_t windowSize = { 500, 312 };

    constexpr uint16_t numSongs = 31;

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
        makeWidget({ 0, 0 }, windowSize, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, widget_type::caption_25, 0, string_ids::music_acknowledgements_caption),
        makeWidget({ windowSize.width - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        makeWidget({ 0, 15 }, { windowSize.width, 297 }, widget_type::panel, 1),
        makeWidget({ 4, 18 }, { windowSize.width - 8, 289 }, widget_type::scrollview, 1, ui::scrollbars::vertical),
        widgetEnd(),
    };

    static window_event_list _events;

    static void initEvents();

    // 0x0043B4AF
    void open()
    {
        if (WindowManager::bringToFront(WindowType::aboutMusic) != nullptr)
            return;

        initEvents();

        auto window = WindowManager::createWindowCentred(
            WindowType::aboutMusic,
            windowSize,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = 1 << widx::close;
        window->initScrollWidgets();

        const auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;
    }

    // 0x0043BFB0
    static void onMouseUp(ui::window* const window, const widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window->type);
                break;
        }
    }

    // 0x0043BFBB
    static void getScrollSize(ui::window*, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        *scrollHeight = numSongs * (10 + 10 + 14);
    }

    // 0x0043BFC0
    static void tooltip(FormatArguments& args, ui::window*, widget_index)
    {
        args.push(string_ids::tooltip_scroll_credits_list);
    }

    // 0x0043B8B8
    static void draw(ui::window* const window, gfx::drawpixelinfo_t* const dpi)
    {
        // Draw widgets.
        window->draw(dpi);
    }

    // 0x0043B8BE
    static void drawScroll(ui::window*, gfx::drawpixelinfo_t* const dpi, uint32_t)
    {
        static const std::pair<string_id, string_id> stringsToDraw[numSongs] = {
            { string_ids::locomotion_title, string_ids::locomotion_title_credit },
            { string_ids::long_dusty_road, string_ids::long_dusty_road_credit },
            { string_ids::flying_high, string_ids::flying_high_credit },
            { string_ids::gettin_on_the_gas, string_ids::gettin_on_the_gas_credit },
            { string_ids::jumpin_the_rails, string_ids::jumpin_the_rails_credit },
            { string_ids::smooth_running, string_ids::smooth_running_credit },
            { string_ids::traffic_jam, string_ids::traffic_jam_credit },
            { string_ids::never_stop_til_you_get_there, string_ids::never_stop_til_you_get_there_credit },
            { string_ids::soaring_away, string_ids::soaring_away_credit },
            { string_ids::techno_torture, string_ids::techno_torture_credit },
            { string_ids::everlasting_high_rise, string_ids::everlasting_high_rise_credit },
            { string_ids::solace, string_ids::solace_credit },
            { string_ids::chrysanthemum, string_ids::chrysanthemum_credit },
            { string_ids::eugenia, string_ids::eugenia_credit },
            { string_ids::the_ragtime_dance, string_ids::the_ragtime_dance_credit },
            { string_ids::easy_winners, string_ids::easy_winners_credit },
            { string_ids::setting_off, string_ids::setting_off_credit },
            { string_ids::a_travellers_seranade, string_ids::a_travellers_seranade_credit },
            { string_ids::latino_trip, string_ids::latino_trip_credit },
            { string_ids::a_good_head_of_steam, string_ids::a_good_head_of_steam_credit },
            { string_ids::hop_to_the_bop, string_ids::hop_to_the_bop_credit },
            { string_ids::the_city_lights, string_ids::the_city_lights_credit },
            { string_ids::steamin_down_town, string_ids::steamin_down_town_credit },
            { string_ids::bright_expectations, string_ids::bright_expectations_credit },
            { string_ids::mo_station, string_ids::mo_station_credit },
            { string_ids::far_out, string_ids::far_out_credit },
            { string_ids::running_on_time, string_ids::running_on_time_credit },
            { string_ids::get_me_to_gladstone_bay, string_ids::get_me_to_gladstone_bay_credit },
            { string_ids::chuggin_along, string_ids::chuggin_along_credit },
            { string_ids::dont_lose_your_rag, string_ids::dont_lose_your_rag_credit },
            { string_ids::sandy_track_blues, string_ids::sandy_track_blues_credit },
        };

        const int16_t x = 240;
        int16_t y = 2;

        for (const auto& songStrings : stringsToDraw)
        {
            // TODO: optimisation: don't draw past fold.

            // Song name
            drawStringCentred(*dpi, x, y, colour::black, songStrings.first, nullptr);
            y += 10;

            // Credit line
            drawStringCentred(*dpi, x, y, colour::black, songStrings.second, nullptr);
            y += 10;

            // Show CS' copyright after every two lines.
            drawStringCentred(*dpi, x, y, colour::black, string_ids::music_copyright, nullptr);
            y += 14;
        }
    }

    static void initEvents()
    {
        _events.on_mouse_up = onMouseUp;
        _events.get_scroll_size = getScrollSize;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
    }
}
