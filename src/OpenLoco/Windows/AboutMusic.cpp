#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::AboutMusic
{
    constexpr Ui::Size windowSize = { 500, 312 };

    constexpr uint16_t numSongs = 31;

    namespace Widx
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

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, windowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::music_acknowledgements_caption),
        makeWidget({ windowSize.width - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { windowSize.width, 297 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 18 }, { windowSize.width - 8, 289 }, WidgetType::scrollview, WindowColour::secondary, Ui::Scrollbars::vertical),
        widgetEnd(),
    };

    static WindowEventList _events;

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
        window->enabledWidgets = 1 << Widx::close;
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);
    }

    // 0x0043BFB0
    static void onMouseUp(Ui::Window* const window, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(window->type);
                break;
        }
    }

    // 0x0043BFBB
    static void getScrollSize(Ui::Window*, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        *scrollHeight = numSongs * (10 + 10 + 14);
    }

    // 0x0043BFC0
    static std::optional<FormatArguments> tooltip(Ui::Window*, WidgetIndex_t)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_credits_list);
        return args;
    }

    // 0x0043B8B8
    static void draw(Ui::Window* const window, Gfx::Context* const context)
    {
        // Draw widgets.
        window->draw(context);
    }

    // 0x0043B8BE
    static void drawScroll(Ui::Window&, Gfx::Context& context, const uint32_t)
    {
        static const std::pair<string_id, string_id> stringsToDraw[numSongs] = {
            { StringIds::locomotion_title, StringIds::locomotion_title_credit },
            { StringIds::long_dusty_road, StringIds::long_dusty_road_credit },
            { StringIds::flying_high, StringIds::flying_high_credit },
            { StringIds::gettin_on_the_gas, StringIds::gettin_on_the_gas_credit },
            { StringIds::jumpin_the_rails, StringIds::jumpin_the_rails_credit },
            { StringIds::smooth_running, StringIds::smooth_running_credit },
            { StringIds::traffic_jam, StringIds::traffic_jam_credit },
            { StringIds::never_stop_til_you_get_there, StringIds::never_stop_til_you_get_there_credit },
            { StringIds::soaring_away, StringIds::soaring_away_credit },
            { StringIds::techno_torture, StringIds::techno_torture_credit },
            { StringIds::everlasting_high_rise, StringIds::everlasting_high_rise_credit },
            { StringIds::solace, StringIds::solace_credit },
            { StringIds::chrysanthemum, StringIds::chrysanthemum_credit },
            { StringIds::eugenia, StringIds::eugenia_credit },
            { StringIds::the_ragtime_dance, StringIds::the_ragtime_dance_credit },
            { StringIds::easy_winners, StringIds::easy_winners_credit },
            { StringIds::setting_off, StringIds::setting_off_credit },
            { StringIds::a_travellers_seranade, StringIds::a_travellers_seranade_credit },
            { StringIds::latino_trip, StringIds::latino_trip_credit },
            { StringIds::a_good_head_of_steam, StringIds::a_good_head_of_steam_credit },
            { StringIds::hop_to_the_bop, StringIds::hop_to_the_bop_credit },
            { StringIds::the_city_lights, StringIds::the_city_lights_credit },
            { StringIds::steamin_down_town, StringIds::steamin_down_town_credit },
            { StringIds::bright_expectations, StringIds::bright_expectations_credit },
            { StringIds::mo_station, StringIds::mo_station_credit },
            { StringIds::far_out, StringIds::far_out_credit },
            { StringIds::running_on_time, StringIds::running_on_time_credit },
            { StringIds::get_me_to_gladstone_bay, StringIds::get_me_to_gladstone_bay_credit },
            { StringIds::chuggin_along, StringIds::chuggin_along_credit },
            { StringIds::dont_lose_your_rag, StringIds::dont_lose_your_rag_credit },
            { StringIds::sandy_track_blues, StringIds::sandy_track_blues_credit },
        };

        const int16_t x = 240;
        int16_t y = 2;

        for (const auto& songStrings : stringsToDraw)
        {
            // TODO: optimisation: don't draw past fold.

            // Song name
            drawStringCentred(context, x, y, Colour::black, songStrings.first, nullptr);
            y += 10;

            // Credit line
            drawStringCentred(context, x, y, Colour::black, songStrings.second, nullptr);
            y += 10;

            // Show CS' copyright after every two lines.
            drawStringCentred(context, x, y, Colour::black, StringIds::music_copyright, nullptr);
            y += 14;
        }
    }

    static void initEvents()
    {
        _events.onMouseUp = onMouseUp;
        _events.getScrollSize = getScrollSize;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
    }
}
