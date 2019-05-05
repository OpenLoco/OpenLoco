#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../scenario.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::LandscapeGeneration
{
    static const gfx::ui_size_t window_size = { 366, 217 };
    static const gfx::ui_size_t land_tab_size = { 366, 232 };

    static loco_global<uint16_t, 0x009C8716> scenarioStartYear;
    static loco_global<uint16_t, 0x009C871A> scenarioFlags;

    static loco_global<uint16_t[10], 0x0112C826> commonFormatArgs;

    namespace common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_options,
            tab_land,
            tab_forests,
            tab_towns,
            tab_industries,
        };

        uint64_t enabled_widgets = (1 << widx::close_button) | (1 << tab_options) | (1 << tab_land) | (1 << tab_forests) | (1 << tab_towns) | (1 << tab_industries);

#define common_options_widgets(frame_height, window_caption_id)                                                                            \
    make_widget({ 0, 0 }, { 366, frame_height }, widget_type::frame, 0),                                                                   \
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, window_caption_id),                                                 \
        make_widget({ 351, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),              \
        make_widget({ 0, 41 }, { 366, 175 }, widget_type::panel, 1),                                                                       \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_landscape_generation_options),  \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_landscape_generation_land),    \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_landscape_generation_forests), \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_landscape_generation_towns),   \
        make_remap_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_landscape_generation_industries)

        // Defined at the bottom of this file.
        static void switchTabWidgets(window* window);
        static void switchTab(window* window, widget_index widgetIndex);

        // 0x0043ECA4
        static void draw_tabs(window* window, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)window;

            call(0x0043ECA4, regs);
        }

        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            window->draw(dpi);
            draw_tabs(window, dpi);
        }

        static void prepare_draw(window* window)
        {
            switchTabWidgets(window);

            window->widgets[widx::frame].right = window->width - 1;
            window->widgets[widx::frame].bottom = window->height - 1;

            window->widgets[widx::panel].right = window->width - 1;
            window->widgets[widx::panel].bottom = window->height - 1;

            window->widgets[widx::caption].right = window->width - 2;

            window->widgets[widx::close_button].left = window->width - 15;
            window->widgets[widx::close_button].right = window->width - 3;
        }
    }

    namespace options
    {
        enum widx
        {
            start_year = 9,
            start_year_up,
            start_year_down,
            generate_when_game_starts,
            generate_now,
        };

        uint64_t enabled_widgets = common::enabled_widgets | (1 << widx::start_year_up) | (1 << widx::start_year_down) | (1 << widx::generate_when_game_starts) | (1 << widx::generate_now);
        uint64_t holdable_widgets = (1 << widx::start_year_up) | (1 << widx::start_year_down);

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_options),
            make_widget({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::uint16_raw),
            make_widget({ 344, 53 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 58 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 10, 68 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::label_generate_random_landscape_when_game_starts, string_ids::tooltip_generate_random_landscape_when_game_starts),
            make_widget({ 196, 200 }, { 160, 12 }, widget_type::wt_11, 1, string_ids::button_generate_landscape, string_ids::tooltip_generate_random_landscape),
            widget_end()
        };

        static window_event_list events;

        // 0x0043DC30
        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::start_year].top,
                colour::black,
                string_ids::start_year,
                nullptr);
        }

        // 0x0043DB76
        static void prepare_draw(window* window)
        {
            common::prepare_draw(window);

            commonFormatArgs[0] = *scenarioStartYear;

            if ((scenarioFlags & scenario::flags::landscape_generation_done) == 0)
            {
                window->activated_widgets |= (1 << widx::generate_when_game_starts);
                window->disabled_widgets |= (1 << widx::generate_now);
            }
            else
            {
                window->activated_widgets &= ~(1 << widx::generate_when_game_starts);
                window->disabled_widgets &= ~(1 << widx::generate_now);
            }
        }

        static void confirmResetLandscape(int32_t eax)
        {
            bool madeAnyChanges = addr<0x009C871C, int32_t>();
            if (madeAnyChanges)
            {
                LandscapeGenerationConfirm::open(eax);
            }
            else
            {
                WindowManager::close(WindowType::landscapeGenerationConfirm, 0);

                // 0x0043EDAD
                *scenarioFlags &= ~(scenario::flags::landscape_generation_done);
                WindowManager::invalidate(WindowType::landscapeGeneration, 0);
                call(0x0043C88C);
                addr<0x009C871C, uint8_t>() = 0;
                addr<0x00F25374, uint8_t>() = 0;
                gfx::invalidate_screen();
            }
        }

        // 0x0043DC83
        static void on_mouse_down(window* window, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::start_year_up:
                    if (*scenarioStartYear + 1 <= scenario::max_year)
                        *scenarioStartYear += 1;
                    window->invalidate();
                    break;

                case widx::start_year_down:
                    if (*scenarioStartYear - 1 >= scenario::min_year)
                        *scenarioStartYear -= 1;
                    window->invalidate();
                    break;
            }
        }

        // 0x0043DC58
        static void on_mouse_up(window* window, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(window);
                    break;

                case common::widx::tab_options:
                case common::widx::tab_land:
                case common::widx::tab_forests:
                case common::widx::tab_towns:
                case common::widx::tab_industries:
                    common::switchTab(window, widgetIndex);
                    break;

                case widx::generate_when_game_starts:
                    if ((scenarioFlags & scenario::landscape_generation_done) == 0)
                    {
                        *scenarioFlags |= scenario::landscape_generation_done;
                        scenario::generateLandscape();
                    }
                    else
                    {
                        WindowManager::closeConstructionWindows();
                        confirmResetLandscape(1);
                    }
                    break;

                case widx::generate_now:
                    confirmResetLandscape(0);
                    break;
            }
        }

        static void init_events()
        {
            events.draw = draw;
            events.prepare_draw = prepare_draw;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
        }
    }

    // 0x0043DA43
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::landscapeGeneration, 0);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::landscapeGeneration, 0);
        }

        // TODO(avgeffen): only needs to be called once.
        options::init_events();

        // Start of 0x0043DAEA
        window = WindowManager::createWindowCentred(WindowType::landscapeGeneration, window_size, 0, &options::events);
        window->widgets = options::widgets;
        window->enabled_widgets = options::enabled_widgets;
        window->number = 0;
        window->current_tab = 0;
        window->frame_no = 0;
        window->row_hover = 0xFFFF;

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_0E;
        // End of 0x0043DAEA

        window->width = window_size.width;
        window->height = window_size.height;

        window->activated_widgets = 0;
        window->holdable_widgets = options::holdable_widgets;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        return window;
    }

    namespace land
    {
        // TODO(avgeffen): widx
        uint64_t enabled_widgets = 0b111011110110111110100;
        uint64_t holdable_widgets = 0b11000110110000000000;

        static widget_t widgets[] = {
            common_options_widgets(232, string_ids::title_landscape_generation_land),
            make_widget({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::sea_level_units),
            make_widget({ 344, 53 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 58 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::min_land_height_units),
            make_widget({ 344, 68 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 73 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 176, 82 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 83 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 256, 97 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::hill_density_pct),
            make_widget({ 344, 98 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 103 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 10, 113 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::create_hills_right_up_to_edge_of_map),
            make_widget({ 4, 127 }, { 358, 100 }, widget_type::scrollview, 1, scrollbars::vertical),
            widget_end()
        };

        static window_event_list events;
    }

    namespace forests
    {
        // TODO(avgeffen): widx
        uint64_t enabled_widgets = 0b110110110110110110110110111110100;
        uint64_t holdable_widgets = 0b10110110110110110110110000000000;

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_forests),
            make_widget({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::number_of_forests_value),
            make_widget({ 344, 53 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 58 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::minimum_forest_radius_blocks),
            make_widget({ 344, 68 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 73 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 82 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::maximum_forest_radius_blocks),
            make_widget({ 344, 83 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 88 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 97 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::minimum_forest_density_pct),
            make_widget({ 344, 98 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 103 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 112 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::maximum_forest_density_pct),
            make_widget({ 344, 113 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 118 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 127 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::number_random_trees_value),
            make_widget({ 344, 128 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 133 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 142 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::min_altitude_for_trees_height),
            make_widget({ 344, 143 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 148 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 256, 157 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::max_altitude_for_trees_height),
            make_widget({ 344, 158 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 163 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            widget_end()
        };

        static window_event_list events;
    }

    namespace towns
    {
        // TODO(avgeffen): widx
        uint64_t enabled_widgets = 0b11110111110100;
        uint64_t holdable_widgets = 0b110000000000;

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_towns),
            make_widget({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::min_land_height_units),
            make_widget({ 344, 53 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 58 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 176, 67 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widget_end()
        };

        static window_event_list events;
    }

    namespace industries
    {
        // TODO(avgeffen): widx
        uint64_t enabled_widgets = 0b1111111110100;
        uint64_t holdable_widgets = 0;

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_towns),
            make_widget({ 176, 52 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 53 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 68 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::allow_industries_to_close_down_during_game),
            make_widget({ 10, 83 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::allow_new_industries_to_start_up_during_game),
            widget_end()
        };

        static window_event_list events;
    };

    namespace common
    {
        static void switchTabWidgets(window* window)
        {
            static widget_t* widgetCollectionsByTabId[] = {
                options::widgets,
                land::widgets,
                forests::widgets,
                towns::widgets,
                industries::widgets,
            };

            widget_t* newWidgets = widgetCollectionsByTabId[window->current_tab];
            if (window->widgets != newWidgets)
            {
                window->widgets = newWidgets;
                window->init_scroll_widgets();
            }

            static const widx tabWidgetIdxByTabId[] = {
                tab_options,
                tab_land,
                tab_forests,
                tab_towns,
                tab_industries,
            };

            window->activated_widgets &= ~((1 << tab_options) | (1 << tab_land) | (1 << tab_forests) | (1 << tab_towns) | (1 << tab_industries));
            window->activated_widgets |= (1ULL << tabWidgetIdxByTabId[window->current_tab]);
        }

        // 0x0043DC98
        static void switchTab(window* window, widget_index widgetIndex)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window->current_tab = widgetIndex - widx::tab_options;
            window->frame_no = 0;
            window->flags &= ~(window_flags::flag_16);
            window->disabled_widgets = 0;

            static uint64_t* enabledWidgetsByTab[] = {
                &options::enabled_widgets,
                &land::enabled_widgets,
                &forests::enabled_widgets,
                &towns::enabled_widgets,
                &industries::enabled_widgets,
            };

            window->enabled_widgets = *enabledWidgetsByTab[window->current_tab];

            static uint64_t* holdableWidgetsByTab[] = {
                &options::holdable_widgets,
                &land::holdable_widgets,
                &forests::holdable_widgets,
                &towns::holdable_widgets,
                &industries::holdable_widgets,
            };

            window->holdable_widgets = *holdableWidgetsByTab[window->current_tab];

            switchTabWidgets(window);

            window->invalidate();

            const gfx::ui_size_t* newSize = &window_size;
            if (widgetIndex == widx::tab_land)
                newSize = &land_tab_size;

            window->min_width = window->max_width = window->width = newSize->width;
            window->min_height = window->max_height = window->height = newSize->height;

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
            window->moveInsideScreenEdges();
        }
    }
}
