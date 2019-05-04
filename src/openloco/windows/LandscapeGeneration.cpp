#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::LandscapeGeneration
{
    static const gfx::ui_size_t window_size = { 366, 217 };

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

        static void init_events()
        {
            events.draw = draw;
            // events.prepare_draw = prepare_draw;
            // events.mouse_up = mouse_up;
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

        window->holdable_widgets = options::holdable_widgets;

        return window;
    }

    namespace land
    {
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
    }

    namespace forests
    {
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
    }

    namespace towns
    {
        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_towns),
            make_widget({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::min_land_height_units),
            make_widget({ 344, 53 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_up),
            make_widget({ 344, 58 }, { 11, 5 }, widget_type::wt_11, 1, string_ids::spinner_down),
            make_widget({ 176, 67 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widget_end()
        };
    }

    namespace industries
    {
        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_towns),
            make_widget({ 176, 52 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 53 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 68 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::allow_industries_to_close_down_during_game),
            make_widget({ 10, 83 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::allow_new_industries_to_start_up_during_game),
            widget_end()
        };
    };

}
