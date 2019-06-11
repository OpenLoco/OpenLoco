#include "../audio/audio.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/land_object.h"
#include "../objects/objectmgr.h"
#include "../s5/s5.h"
#include "../scenario.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::LandscapeGeneration
{
    static const gfx::ui_size_t window_size = { 366, 217 };
    static const gfx::ui_size_t land_tab_size = { 366, 232 };

    static const uint8_t rowHeight = 22; // CJK: 22

    static loco_global<uint16_t, 0x00525FB2> seaLevel;

    static loco_global<uint8_t, 0x00525FB6> primaryLandObjectIndex;

    static loco_global<uint8_t, 0x00526247> industryFlags;

    static constexpr size_t maxLandObjects = objectmgr::get_max_objects(object_type::land);

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

        const uint64_t enabled_widgets = (1 << widx::close_button) | (1 << tab_options) | (1 << tab_land) | (1 << tab_forests) | (1 << tab_towns) | (1 << tab_industries);

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
        static void initEvents();
        static void switchTabWidgets(window* window);
        static void switchTab(window* window, widget_index widgetIndex);

        // 0x0043ECA4
        static void draw_tabs(window* window, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Options tab
            {
                static const uint32_t optionTabImageIds[] = {
                    interface_skin::image_ids::tab_cogs_frame0,
                    interface_skin::image_ids::tab_cogs_frame1,
                    interface_skin::image_ids::tab_cogs_frame2,
                    interface_skin::image_ids::tab_cogs_frame3,
                };

                uint32_t imageId = skin->img;
                if (window->current_tab == widx::tab_options - widx::tab_options)
                    imageId += optionTabImageIds[(window->frame_no / 2) % std::size(optionTabImageIds)];
                else
                    imageId += optionTabImageIds[0];

                widget::draw_tab(window, dpi, imageId, widx::tab_options);
            }

            // Land tab
            {
                auto land = objectmgr::get<land_object>(*primaryLandObjectIndex);
                const uint32_t imageId = land->var_16 + land::image_ids::toolbar_terraform_land;
                widget::draw_tab(window, dpi, imageId, widx::tab_land);
            }

            // Forest tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::toolbar_menu_plant_trees;
                widget::draw_tab(window, dpi, imageId, widx::tab_forests);
            }

            // Towns tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::toolbar_menu_towns;
                widget::draw_tab(window, dpi, imageId, widx::tab_towns);
            }

            // Industries tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::toolbar_menu_industries;
                widget::draw_tab(window, dpi, imageId, widx::tab_industries);
            }
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

        static void update(window* window)
        {
            window->frame_no++;
            window->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::landscapeGeneration, window->number, window->current_tab + widx::tab_options);
        }
    }

    namespace options
    {
        enum widx
        {
            start_year = 9,
            start_year_down,
            start_year_up,
            generate_when_game_starts,
            generate_now,
        };

        const uint64_t enabled_widgets = common::enabled_widgets | (1 << widx::start_year_up) | (1 << widx::start_year_down) | (1 << widx::generate_when_game_starts) | (1 << widx::generate_now);
        const uint64_t holdable_widgets = (1 << widx::start_year_up) | (1 << widx::start_year_down);

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_options),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::start_year_value),
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
                string_ids::start_year);
        }

        // 0x0043DB76
        static void prepare_draw(window* window)
        {
            common::prepare_draw(window);

            commonFormatArgs[0] = s5::getOptions().scenarioStartYear;

            if ((s5::getOptions().scenarioFlags & scenario::flags::landscape_generation_done) == 0)
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

        static void confirmResetLandscape(int32_t promptType)
        {
            if (s5::getOptions().madeAnyChanges)
            {
                LandscapeGenerationConfirm::open(promptType);
            }
            else
            {
                WindowManager::close(WindowType::landscapeGenerationConfirm, 0);

                if (promptType == 0)
                    scenario::generateLandscape();
                else
                    scenario::eraseLandscape();
            }
        }

        // 0x0043DC83
        static void on_mouse_down(window* window, widget_index widgetIndex)
        {
            auto& options = s5::getOptions();

            switch (widgetIndex)
            {
                case widx::start_year_up:
                    if (options.scenarioStartYear + 1 <= scenario::max_year)
                        options.scenarioStartYear += 1;
                    window->invalidate();
                    break;

                case widx::start_year_down:
                    if (options.scenarioStartYear - 1 >= scenario::min_year)
                        options.scenarioStartYear -= 1;
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
                    if ((s5::getOptions().scenarioFlags & scenario::landscape_generation_done) == 0)
                    {
                        s5::getOptions().scenarioFlags |= scenario::landscape_generation_done;
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

        static void initEvents()
        {
            events.draw = draw;
            events.prepare_draw = prepare_draw;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
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
        common::initEvents();

        // Start of 0x0043DAEA
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(WindowType::landscapeGeneration, window_size, 0, &options::events);
            window->widgets = options::widgets;
            window->enabled_widgets = options::enabled_widgets;
            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->row_hover = -1;

            auto interface = objectmgr::get<interface_skin_object>();
            window->colours[0] = interface->colour_0B;
            window->colours[1] = interface->colour_0E;
        }
        // End of 0x0043DAEA

        window->width = window_size.width;
        window->height = window_size.height;

        window->invalidate();

        window->activated_widgets = 0;
        window->holdable_widgets = options::holdable_widgets;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        return window;
    }

    namespace land
    {
        enum widx
        {
            sea_level = 9,
            sea_level_down,
            sea_level_up,
            min_land_height,
            min_land_height_down,
            min_land_height_up,
            topography_style,
            topography_style_btn,
            hill_density,
            hill_density_down,
            hill_density_up,
            hills_edge_of_map,
            scrollview,
        };

        const uint64_t enabled_widgets = common::enabled_widgets | (1 << widx::sea_level_up) | (1 << widx::sea_level_down) | (1 << widx::min_land_height_up) | (1 << widx::min_land_height_down) | (1 << widx::topography_style) | (1 << widx::topography_style_btn) | (1 << widx::hill_density_up) | (1 << widx::hill_density_down) | (1 << widx::hills_edge_of_map);
        const uint64_t holdable_widgets = (1 << widx::sea_level_up) | (1 << widx::sea_level_down) | (1 << widx::min_land_height_up) | (1 << widx::min_land_height_down) | (1 << widx::hill_density_up) | (1 << widx::hill_density_down);

        static widget_t widgets[] = {
            common_options_widgets(232, string_ids::title_landscape_generation_land),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::sea_level_units),
            make_stepper_widgets({ 256, 67 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::min_land_height_units),
            make_widget({ 176, 82 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 83 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_stepper_widgets({ 256, 97 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::hill_density_percent),
            make_widget({ 10, 113 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::create_hills_right_up_to_edge_of_map),
            make_widget({ 4, 127 }, { 358, 100 }, widget_type::scrollview, 1, scrollbars::vertical),
            widget_end()
        };

        static window_event_list events;

        // 0x0043DF89
        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::sea_level].top,
                colour::black,
                string_ids::sea_level);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::min_land_height].top,
                colour::black,
                string_ids::min_land_height);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::topography_style].top,
                colour::black,
                string_ids::topography_style);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::hill_density].top,
                colour::black,
                string_ids::hill_density);
        }

        static const string_id landDistributionLabelIds[] = {
            string_ids::land_distribution_everywhere,
            string_ids::land_distribution_nowhere,
            string_ids::land_distribution_far_from_water,
            string_ids::land_distribution_near_water,
            string_ids::land_distribution_on_mountains,
            string_ids::land_distribution_far_from_mountains,
            string_ids::land_distribution_in_small_random_areas,
            string_ids::land_distribution_in_large_random_areas,
            string_ids::land_distribution_around_cliffs,
        };

        // 0x0043E01C
        static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < maxLandObjects; i++)
            {
                auto landObject = objectmgr::get<land_object>(i);
                if (landObject == nullptr)
                    continue;

                // Draw tile icon.
                const uint32_t imageId = landObject->var_16 + openloco::land::image_ids::landscape_generator_tile_icon;
                gfx::draw_image(dpi, 2, yPos + 1, imageId);

                // Draw land description.
                commonFormatArgs[0] = landObject->name;
                gfx::draw_string_494BBF(*dpi, 24, yPos + 5, 121, colour::black, string_ids::wcolour2_stringid2, &*commonFormatArgs);

                // Draw rectangle.
                gfx::fill_rect_inset(dpi, 150, yPos + 5, 340, yPos + 16, window->colours[1], 0b110000);

                // Draw current distribution setting.
                const string_id distributionId = landDistributionLabelIds[s5::getOptions().landDistributionPatterns[i]];
                commonFormatArgs[0] = distributionId;
                gfx::draw_string_494BBF(*dpi, 151, yPos + 5, 177, colour::black, string_ids::white_stringid2, &*commonFormatArgs);

                // Draw rectangle (knob).
                const uint8_t flags = window->row_hover == i ? 0b110000 : 0;
                gfx::fill_rect_inset(dpi, 329, yPos + 6, 339, yPos + 15, window->colours[1], flags);

                // Draw triangle (knob).
                gfx::draw_string_494B3F(*dpi, 330, yPos + 6, colour::black, string_ids::dropdown, nullptr);

                yPos += rowHeight;
            }
        }

        // 0x0043E2AC
        static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = 0;

            for (uint16_t i = 0; i < maxLandObjects; i++)
            {
                auto landObject = objectmgr::get<land_object>(i);
                if (landObject == nullptr)
                    continue;

                *scrollHeight += rowHeight;
            }
        }

        static const string_id topographyStyleIds[] = {
            string_ids::flat_land,
            string_ids::small_hills,
            string_ids::mountains,
            string_ids::half_mountains_half_hills,
            string_ids::half_mountains_half_flat,
        };

        // 0x0043E1BA
        static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::topography_style_btn:
                    if (itemIndex != -1)
                    {
                        s5::getOptions().topographyStyle = itemIndex;
                        window->invalidate();
                    }
                    break;

                case widx::scrollview:
                    if (itemIndex != -1 && window->row_hover != -1)
                    {
                        s5::getOptions().landDistributionPatterns[window->row_hover] = itemIndex;
                        window->invalidate();
                    }
                    break;
            }
        }

        // 0x0043E173
        static void on_mouse_down(window* window, widget_index widgetIndex)
        {
            auto& options = s5::getOptions();

            switch (widgetIndex)
            {
                case widx::sea_level_up:
                    *seaLevel = std::min<int8_t>(*seaLevel + 1, scenario::max_sea_level);
                    break;

                case widx::sea_level_down:
                    *seaLevel = std::max<int8_t>(scenario::min_sea_level, *seaLevel - 1);
                    break;

                case widx::min_land_height_up:
                    options.minLandHeight = std::min<int8_t>(options.minLandHeight + 1, scenario::max_base_land_height);
                    break;

                case widx::min_land_height_down:
                    options.minLandHeight = std::max<int8_t>(scenario::min_base_land_height, options.minLandHeight - 1);
                    break;

                case widx::topography_style_btn:
                {
                    widget_t& target = window->widgets[widx::topography_style];
                    dropdown::show(window->x + target.left, window->y + target.top, target.width() - 4, target.height(), window->colours[1], std::size(topographyStyleIds), 0x80);

                    for (size_t i = 0; i < std::size(topographyStyleIds); i++)
                        dropdown::add(i, topographyStyleIds[i]);

                    dropdown::set_highlighted_item(options.topographyStyle);
                    break;
                }

                case widx::hill_density_up:
                    options.hillDensity = std::min<int8_t>(options.hillDensity + 1, scenario::max_hill_density);
                    break;

                case widx::hill_density_down:
                    options.hillDensity = std::max<int8_t>(scenario::min_hill_density, options.hillDensity - 1);
                    break;

                default:
                    // Nothing was changed, don't invalidate.
                    return;
            }

            // After changing any of the options, invalidate the window.
            window->invalidate();
        }

        // 0x0043E14E
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

                case widx::hills_edge_of_map:
                    s5::getOptions().scenarioFlags ^= scenario::flags::hills_edge_of_map;
                    window->invalidate();
                    break;
            }
        }

        // 0x0043E421
        static int16_t scrollPosToLandIndex(int16_t xPos, int16_t yPos)
        {
            if (xPos < 150)
                return -1;

            for (uint16_t i = 0; i < maxLandObjects; i++)
            {
                auto landObject = objectmgr::get<land_object>(i);
                if (landObject == nullptr)
                    continue;

                yPos -= rowHeight;
                if (yPos < 0)
                    return i;
            }

            return -1;
        }

        // 0x0043E1CF
        static void scroll_mouse_down(window* window, int16_t xPos, int16_t yPos, uint8_t scrollIndex)
        {
            int16_t landIndex = scrollPosToLandIndex(xPos, yPos);
            if (landIndex == -1)
                return;

            window->row_hover = landIndex;

            audio::play_sound(audio::sound_id::click_down, window->widgets[widx::scrollview].right);

            const widget_t& target = window->widgets[widx::scrollview];
            const int16_t dropdownX = window->x + target.left + 151;
            const int16_t dropdownY = window->y + target.top + 6 + landIndex * rowHeight - window->scroll_areas[0].v_top;
            dropdown::show(dropdownX, dropdownY, 188, 12, window->colours[1], std::size(landDistributionLabelIds), 0x80);

            for (size_t i = 0; i < std::size(landDistributionLabelIds); i++)
                dropdown::add(i, string_ids::dropdown_stringid, landDistributionLabelIds[i]);

            dropdown::set_item_selected(s5::getOptions().landDistributionPatterns[landIndex]);
        }

        // 0x0043DEBF
        static void prepare_draw(window* window)
        {
            common::prepare_draw(window);

            commonFormatArgs[0] = *seaLevel;
            auto& options = s5::getOptions();
            commonFormatArgs[1] = options.minLandHeight;
            commonFormatArgs[2] = options.hillDensity;

            window->widgets[widx::topography_style].text = topographyStyleIds[options.topographyStyle];

            if ((options.scenarioFlags & scenario::flags::hills_edge_of_map) != 0)
                window->activated_widgets |= (1 << widx::hills_edge_of_map);
            else
                window->activated_widgets &= ~(1 << widx::hills_edge_of_map);
        }

        // 0x0043E2A2
        static void tooltip(ui::window* window, widget_index widgetIndex)
        {
            commonFormatArgs[0] = string_ids::tooltip_scroll_list;
        }

        // 0x0043E3D9
        static void update(window* window)
        {
            common::update(window);

            auto dropdown = WindowManager::find(WindowType::dropdown, 0);
            if (dropdown == nullptr && window->row_hover != -1)
            {
                window->row_hover = -1;
                window->invalidate();
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.draw_scroll = draw_scroll;
            events.get_scroll_size = get_scroll_size;
            events.prepare_draw = prepare_draw;
            events.on_dropdown = on_dropdown;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = update;
            events.scroll_mouse_down = scroll_mouse_down;
            events.tooltip = tooltip;
        }
    }

    namespace forests
    {
        enum widx
        {
            number_of_forests = 9,
            number_of_forests_down,
            number_of_forests_up,
            min_forest_radius,
            min_forest_radius_down,
            min_forest_radius_up,
            max_forest_radius,
            max_forest_radius_down,
            max_forest_radius_up,
            min_forest_density,
            min_forest_density_down,
            min_forest_density_up,
            max_forest_density,
            max_forest_density_down,
            max_forest_density_up,
            number_random_trees,
            number_random_trees_down,
            number_random_trees_up,
            min_altitude_for_trees,
            min_altitude_for_trees_down,
            min_altitude_for_trees_up,
            max_altitude_for_trees,
            max_altitude_for_trees_down,
            max_altitude_for_trees_up,
        };

        // TODO(avgeffen) shift overflow for last widget. Should fit in uint64_t, but problematic on 32-bits arch?
        const uint64_t enabled_widgets = common::enabled_widgets | (1 << widx::number_of_forests_up) | (1 << widx::number_of_forests_down) | (1 << widx::min_forest_radius_up) | (1 << widx::min_forest_radius_down) | (1 << widx::max_forest_radius_up) | (1 << widx::max_forest_radius_down) | (1 << widx::min_forest_density_up) | (1 << widx::min_forest_density_down) | (1 << widx::max_forest_density_up) | (1 << widx::max_forest_density_down) | (1 << widx::number_random_trees_up) | (1 << widx::number_random_trees_down) | (1 << widx::min_altitude_for_trees_up) | (1 << widx::min_altitude_for_trees_down) | (1 << widx::max_altitude_for_trees_down); // | (1 << widx::max_altitude_for_trees_up);
        const uint64_t holdable_widgets = (1 << widx::number_of_forests_up) | (1 << widx::number_of_forests_down) | (1 << widx::min_forest_radius_up) | (1 << widx::min_forest_radius_down) | (1 << widx::max_forest_radius_up) | (1 << widx::max_forest_radius_down) | (1 << widx::min_forest_density_up) | (1 << widx::min_forest_density_down) | (1 << widx::max_forest_density_up) | (1 << widx::max_forest_density_down) | (1 << widx::number_random_trees_up) | (1 << widx::number_random_trees_down) | (1 << widx::min_altitude_for_trees_up) | (1 << widx::min_altitude_for_trees_down) | (1 << widx::max_altitude_for_trees_down);                          // | (1 << widx::max_altitude_for_trees_up);

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_forests),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::number_of_forests_value),
            make_stepper_widgets({ 256, 67 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::min_forest_radius_blocks),
            make_stepper_widgets({ 256, 82 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::max_forest_radius_blocks),
            make_stepper_widgets({ 256, 97 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::min_forest_density_percent),
            make_stepper_widgets({ 256, 112 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::max_forest_density_percent),
            make_stepper_widgets({ 256, 127 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::number_random_trees_value),
            make_stepper_widgets({ 256, 142 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::min_altitude_for_trees_height),
            make_stepper_widgets({ 256, 157 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::max_altitude_for_trees_height),
            widget_end()
        };

        static window_event_list events;

        // 0x0043E53A
        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::number_of_forests].top,
                colour::black,
                string_ids::number_of_forests);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::min_forest_radius].top,
                colour::black,
                string_ids::min_forest_radius);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::max_forest_radius].top,
                colour::black,
                string_ids::max_forest_radius);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::min_forest_density].top,
                colour::black,
                string_ids::min_forest_density);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::max_forest_density].top,
                colour::black,
                string_ids::max_forest_density);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::number_random_trees].top,
                colour::black,
                string_ids::number_random_trees);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::min_altitude_for_trees].top,
                colour::black,
                string_ids::min_altitude_for_trees);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::max_altitude_for_trees].top,
                colour::black,
                string_ids::max_altitude_for_trees);
        }

        // 0x0043E670
        static void on_mouse_down(window* window, widget_index widgetIndex)
        {
            auto& options = s5::getOptions();

            switch (widgetIndex)
            {
                case widx::number_of_forests_up:
                {
                    options.numberOfForests = std::min<int16_t>(options.numberOfForests + 10, scenario::max_num_forests);
                    break;
                }
                case widx::number_of_forests_down:
                {
                    options.numberOfForests = std::max<int16_t>(scenario::min_num_forests, options.numberOfForests - 10);
                    break;
                }
                case widx::min_forest_radius_up:
                {
                    options.minForestRadius = std::min<int16_t>(options.minForestRadius + 1, scenario::max_forest_radius);
                    if (options.minForestRadius > options.maxForestRadius)
                        options.maxForestRadius = options.minForestRadius;
                    break;
                }
                case widx::min_forest_radius_down:
                {
                    options.minForestRadius = std::max<int8_t>(scenario::min_forest_radius, options.minForestRadius - 1);
                    break;
                }
                case widx::max_forest_radius_up:
                {
                    options.maxForestRadius = std::clamp<int8_t>(options.maxForestRadius + 1, scenario::min_forest_radius, scenario::max_forest_radius);
                    break;
                }
                case widx::max_forest_radius_down:
                {
                    options.maxForestRadius = std::clamp<int8_t>(options.maxForestRadius - 1, scenario::min_forest_radius, scenario::max_forest_radius);
                    if (options.maxForestRadius < options.minForestRadius)
                        options.minForestRadius = options.maxForestRadius;
                    break;
                }
                case widx::min_forest_density_up:
                {
                    options.minForestDensity = std::min<int8_t>(options.minForestDensity + 1, scenario::max_forest_density);
                    if (options.minForestDensity > options.maxForestDensity)
                        options.maxForestDensity = options.minForestDensity;
                    break;
                }
                case widx::min_forest_density_down:
                {
                    options.minForestDensity = std::max<int8_t>(scenario::min_forest_density, options.minForestDensity - 1);
                    break;
                }
                case widx::max_forest_density_up:
                {
                    options.maxForestDensity = std::min<int8_t>(options.maxForestDensity + 1, scenario::max_forest_density);
                    break;
                }
                case widx::max_forest_density_down:
                {
                    options.maxForestDensity = std::max<int8_t>(scenario::min_forest_density, options.maxForestDensity - 1);
                    if (options.maxForestDensity < options.minForestDensity)
                        options.minForestDensity = options.maxForestDensity;
                    break;
                }
                case widx::number_random_trees_up:
                {
                    options.numberRandomTrees = std::min<int16_t>(options.numberRandomTrees + 25, scenario::max_num_trees);
                    break;
                }
                case widx::number_random_trees_down:
                {
                    options.numberRandomTrees = std::max<int16_t>(scenario::min_num_trees, options.numberRandomTrees - 25);
                    break;
                }
                case widx::min_altitude_for_trees_up:
                {
                    options.minAltitudeForTrees = std::min<int8_t>(options.minAltitudeForTrees + 1, scenario::max_altitude_trees);
                    if (options.minAltitudeForTrees > options.maxAltitudeForTrees)
                        options.maxAltitudeForTrees = options.minAltitudeForTrees;
                    break;
                }
                case widx::min_altitude_for_trees_down:
                {
                    options.minAltitudeForTrees = std::max<int8_t>(scenario::min_altitude_trees, options.minAltitudeForTrees - 1);
                    break;
                }
                case widx::max_altitude_for_trees_up:
                {
                    options.maxAltitudeForTrees = std::min<int8_t>(options.maxAltitudeForTrees + 1, scenario::max_altitude_trees);
                    break;
                }
                case widx::max_altitude_for_trees_down:
                {
                    options.maxAltitudeForTrees = std::max<int8_t>(scenario::min_altitude_trees, options.maxAltitudeForTrees - 1);
                    if (options.maxAltitudeForTrees < options.minAltitudeForTrees)
                        options.minAltitudeForTrees = options.maxAltitudeForTrees;
                    break;
                }
                default:
                    // Nothing was changed, don't invalidate.
                    return;
            }

            // After changing any of the options, invalidate the window.
            window->invalidate();
        }

        // 0x0043E655
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
            }
        }

        // 0x0043E44F
        static void prepare_draw(window* window)
        {
            common::prepare_draw(window);

            auto& options = s5::getOptions();
            commonFormatArgs[0] = options.numberOfForests;
            commonFormatArgs[1] = options.minForestRadius;
            commonFormatArgs[2] = options.maxForestRadius;
            commonFormatArgs[3] = options.minForestDensity * 14;
            commonFormatArgs[4] = options.maxForestDensity * 14;
            commonFormatArgs[5] = options.numberRandomTrees;
            commonFormatArgs[6] = options.minAltitudeForTrees;
            commonFormatArgs[7] = options.maxAltitudeForTrees;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.prepare_draw = prepare_draw;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
        }
    }

    namespace towns
    {
        enum widx
        {
            number_of_towns = 9,
            number_of_towns_down,
            number_of_towns_up,
            max_town_size,
            max_town_size_btn,
        };

        const uint64_t enabled_widgets = common::enabled_widgets | (1 << widx::number_of_towns_up) | (1 << widx::number_of_towns_down) | (1 << widx::max_town_size) | (1 << widx::max_town_size_btn);
        const uint64_t holdable_widgets = (1 << widx::number_of_towns_up) | (1 << widx::number_of_towns_down);

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_towns),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_18, 1, string_ids::number_of_towns_value),
            make_widget({ 176, 67 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widget_end()
        };

        static window_event_list events;

        // 0x0043E9A3
        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::number_of_towns].top,
                colour::black,
                string_ids::number_of_towns);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::max_town_size].top,
                colour::black,
                string_ids::max_town_size);
        }

        static const string_id townSizeLabels[] = {
            string_ids::town_size_1,
            string_ids::town_size_2,
            string_ids::town_size_3,
            string_ids::town_size_4,
            string_ids::town_size_5,
            string_ids::town_size_6,
            string_ids::town_size_7,
            string_ids::town_size_8,
        };

        // 0x0043EBF8
        static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::max_town_size_btn || itemIndex == -1)
                return;

            s5::getOptions().maxTownSize = itemIndex;
            window->invalidate();
        }

        // 0x0043EBF1
        static void on_mouse_down(window* window, widget_index widgetIndex)
        {
            auto& options = s5::getOptions();

            switch (widgetIndex)
            {
                case widx::number_of_towns_up:
                {
                    uint16_t newNumTowns = std::min<uint16_t>(options.numberOfTowns + 1, townmgr::max_towns);
                    options.numberOfTowns = newNumTowns;
                    window->invalidate();
                    break;
                }

                case widx::number_of_towns_down:
                {
                    uint16_t newNumTowns = std::max<uint16_t>(0, options.numberOfTowns - 1);
                    options.numberOfTowns = newNumTowns;
                    window->invalidate();
                    break;
                }

                case widx::max_town_size_btn:
                {
                    widget_t& target = window->widgets[widx::max_town_size];
                    dropdown::show(window->x + target.left, window->y + target.top, target.width() - 4, target.height(), window->colours[1], std::size(townSizeLabels), 0x80);

                    for (size_t i = 0; i < std::size(townSizeLabels); i++)
                        dropdown::add(i, townSizeLabels[i]);

                    dropdown::set_highlighted_item(options.maxTownSize);
                    break;
                }
            }
        }

        // 0x0043EBCA
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
            }
        }

        // 0x0043EAEB
        static void prepare_draw(window* window)
        {
            common::prepare_draw(window);

            commonFormatArgs[0] = s5::getOptions().numberOfTowns;

            widgets[widx::max_town_size].text = townSizeLabels[s5::getOptions().maxTownSize];
        }

        static void initEvents()
        {
            events.draw = draw;
            events.prepare_draw = prepare_draw;
            events.on_dropdown = on_dropdown;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
        }
    }

    namespace industries
    {
        enum widx
        {
            num_industries = 9,
            num_industries_btn,
            check_allow_industries_close_down,
            check_allow_industries_start_up,
        };

        const uint64_t enabled_widgets = common::enabled_widgets | (1 << widx::num_industries) | (1 << widx::num_industries_btn) | (1 << widx::check_allow_industries_close_down) | (1 << widx::check_allow_industries_start_up);
        const uint64_t holdable_widgets = 0;

        static widget_t widgets[] = {
            common_options_widgets(217, string_ids::title_landscape_generation_industries),
            make_widget({ 176, 52 }, { 180, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 53 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 68 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::allow_industries_to_close_down_during_game),
            make_widget({ 10, 83 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::allow_new_industries_to_start_up_during_game),
            widget_end()
        };

        static window_event_list events;

        // 0x0043EB9D
        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            gfx::draw_string_494B3F(
                *dpi,
                window->x + 10,
                window->y + window->widgets[widx::num_industries].top,
                colour::black,
                string_ids::number_of_industries);
        }

        static const string_id numIndustriesLabels[] = {
            string_ids::industry_size_low,
            string_ids::industry_size_medium,
            string_ids::industry_size_high,
        };

        // 0x0043EBF8
        static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::num_industries_btn || itemIndex == -1)
                return;

            s5::getOptions().numberOfIndustries = itemIndex;
            window->invalidate();
        }

        // 0x0043EBF1
        static void on_mouse_down(window* window, widget_index widgetIndex)
        {
            if (widgetIndex != widx::num_industries_btn)
                return;

            widget_t& target = window->widgets[widx::num_industries];
            dropdown::show(window->x + target.left, window->y + target.top, target.width() - 4, target.height(), window->colours[1], std::size(numIndustriesLabels), 0x80);

            for (size_t i = 0; i < std::size(numIndustriesLabels); i++)
                dropdown::add(i, numIndustriesLabels[i]);

            dropdown::set_highlighted_item(s5::getOptions().numberOfIndustries);
        }

        // 0x0043EBCA
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

                case widx::check_allow_industries_close_down:
                    *industryFlags ^= scenario::industry_flags::allow_industries_close_down;
                    window->invalidate();
                    break;

                case widx::check_allow_industries_start_up:
                    *industryFlags ^= scenario::industry_flags::allow_industries_start_up;
                    window->invalidate();
                    break;
            }
        }

        // 0x0043EAEB
        static void prepare_draw(window* window)
        {
            common::prepare_draw(window);

            widgets[widx::num_industries].text = numIndustriesLabels[s5::getOptions().numberOfIndustries];

            window->activated_widgets &= ~((1 << widx::check_allow_industries_close_down) | (1 << widx::check_allow_industries_start_up));
            if (industryFlags & scenario::industry_flags::allow_industries_close_down)
                window->activated_widgets |= 1 << widx::check_allow_industries_close_down;
            if (industryFlags & scenario::industry_flags::allow_industries_start_up)
                window->activated_widgets |= 1 << widx::check_allow_industries_start_up;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.prepare_draw = prepare_draw;
            events.on_dropdown = on_dropdown;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
        }
    };

    namespace common
    {
        static void initEvents()
        {
            options::initEvents();
            land::initEvents();
            forests::initEvents();
            towns::initEvents();
            industries::initEvents();
        }

        static void switchTabWidgets(window* window)
        {
            window->activated_widgets = 0;

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

            static const uint64_t* enabledWidgetsByTab[] = {
                &options::enabled_widgets,
                &land::enabled_widgets,
                &forests::enabled_widgets,
                &towns::enabled_widgets,
                &industries::enabled_widgets,
            };

            window->enabled_widgets = *enabledWidgetsByTab[window->current_tab];

            static const uint64_t* holdableWidgetsByTab[] = {
                &options::holdable_widgets,
                &land::holdable_widgets,
                &forests::holdable_widgets,
                &towns::holdable_widgets,
                &industries::holdable_widgets,
            };

            window->holdable_widgets = *holdableWidgetsByTab[window->current_tab];

            static window_event_list* eventsByTab[] = {
                &options::events,
                &land::events,
                &forests::events,
                &towns::events,
                &industries::events,
            };

            window->event_handlers = eventsByTab[window->current_tab];

            switchTabWidgets(window);

            window->invalidate();

            const gfx::ui_size_t* newSize = &window_size;
            if (widgetIndex == widx::tab_land)
                newSize = &land_tab_size;

            window->set_size(*newSize);

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
            window->invalidate();
            window->moveInsideScreenEdges();
        }
    }
}
