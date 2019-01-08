#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../stationmgr.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::station_list
{
    static const gfx::ui_size_t window_size = { 600, 197 };
    static const gfx::ui_size_t max_dimensions = { 640, 1200 };
    static const gfx::ui_size_t min_dimensions = { 192, 100 };

    static const uint8_t rowHeight = 10; // CJK: 13

    enum widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_all_stations,
        tab_rail_stations,
        tab_road_stations,
        tab_airports,
        tab_ship_ports,
        company_select,
        sort_name,
        sort_status,
        sort_total_waiting,
        sort_accepts,
        scrollview,
    };

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 600, 197 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 598, 13 }, widget_type::caption_24, 0, string_ids::stringid_all_stations),
        make_widget({ 585, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 600, 155 }, widget_type::panel, 1),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_all_stations),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_rail_stations),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_road_stations),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_airports),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_ship_ports),
        make_widget({ 0, 14 }, { 26, 26 }, widget_type::wt_9, 0, string_ids::null, string_ids::tooltip_select_company),
        make_widget({ 4, 43 }, { 200, 12 }, widget_type::wt_14, 1, string_ids::null, string_ids::tooltip_sort_by_name),
        make_widget({ 204, 43 }, { 200, 12 }, widget_type::wt_14, 1, string_ids::null, string_ids::tooltip_sort_by_station_status),
        make_widget({ 404, 43 }, { 90, 12 }, widget_type::wt_14, 1, string_ids::null, string_ids::tooltip_sort_by_total_units_waiting),
        make_widget({ 494, 43 }, { 120, 12 }, widget_type::wt_14, 1, string_ids::null, string_ids::tooltip_sort_by_cargo_accepted),
        make_widget({ 3, 56 }, { 594, 126 }, widget_type::scrollview, 1, scrollbars::vertical),
        widget_end(),
    };

    static window_event_list _events;

    struct TabDetails
    {
        widx widgetIndex;
        string_id windowTitleId;
        uint32_t imageId;
        uint16_t stationMask;
    };

    static TabDetails tabInformationByType[] = {
        { tab_all_stations, string_ids::stringid_all_stations, interface_skin::image_ids::all_stations, station_mask_all_modes },
        { tab_rail_stations, string_ids::stringid_rail_stations, interface_skin::image_ids::rail_stations, station_flags::transport_mode_rail },
        { tab_road_stations, string_ids::stringid_road_stations, interface_skin::image_ids::road_stations, station_flags::transport_mode_road },
        { tab_airports, string_ids::stringid_airports, interface_skin::image_ids::airports, station_flags::transport_mode_air },
        { tab_ship_ports, string_ids::stringid_ship_ports, interface_skin::image_ids::ship_ports, station_flags::transport_mode_water }
    };

    enum SortMode : uint16_t
    {
        Name,
        Status,
        TotalUnitsWaiting,
        CargoAccepted,
    };

    loco_global<uint16_t[4], 0x112C826> _common_format_args;

    static ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void event_08(window* window);
    static void event_09(window* window);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex);
    static void on_mouse_down(ui::window* window, widget_index widgetIndex);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_update(window* window);
    static void prepare_draw(ui::window* window);
    static void tooltip(ui::window* window, widget_index widgetIndex);

    static void init_events()
    {
        _events.cursor = cursor;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;
        _events.event_08 = event_08;
        _events.event_09 = event_09;
        _events.get_scroll_size = get_scroll_size;
        _events.on_dropdown = on_dropdown;
        _events.on_mouse_down = on_mouse_down;
        _events.on_mouse_up = on_mouse_up;
        _events.on_update = on_update;
        _events.scroll_mouse_down = on_scroll_mouse_down;
        _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.prepare_draw = prepare_draw;
        _events.tooltip = tooltip;
    }

    // 0x004910E8
    static void refreshStationList(window* window)
    {
        window->row_count = 0;

        for (auto& station : stationmgr::stations())
        {
            if (station.empty())
                continue;

            if (station.owner == window->number)
            {
                station.flags &= ~station_flags::flag_4;
            }
        }
    }

    // 0x004911FD
    static bool orderByName(const openloco::station& lhs, const openloco::station& rhs)
    {
        char lhsString[256] = { 0 };
        stringmgr::format_string(lhsString, lhs.name, (void*)&lhs.town);

        char rhsString[256] = { 0 };
        stringmgr::format_string(rhsString, rhs.name, (void*)&rhs.town);

        return strcmp(lhsString, rhsString) < 0;
    }

    // 0x00491281, 0x00491247
    static bool orderByQuantity(const openloco::station& lhs, const openloco::station& rhs)
    {
        uint32_t lhsSum = 0;
        for (auto cargo : lhs.cargo_stats)
        {
            lhsSum += cargo.quantity;
        }

        uint32_t rhsSum = 0;
        for (auto cargo : rhs.cargo_stats)
        {
            rhsSum += cargo.quantity;
        }

        return rhsSum < lhsSum;
    }

    // 0x004912BB
    static bool orderByAccepts(const openloco::station& lhs, const openloco::station& rhs)
    {
        char* ptr;

        char lhsString[256] = { 0 };
        ptr = &lhsString[0];
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            if (lhs.cargo_stats[cargoId].is_accepted())
            {
                ptr = stringmgr::format_string(ptr, objectmgr::get<cargo_object>(cargoId)->name);
            }
        }

        char rhsString[256] = { 0 };
        ptr = &rhsString[0];
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            if (rhs.cargo_stats[cargoId].is_accepted())
            {
                ptr = stringmgr::format_string(ptr, objectmgr::get<cargo_object>(cargoId)->name);
            }
        }

        return strcmp(lhsString, rhsString) < 0;
    }

    // 0x004911FD, 0x00491247, 0x00491281, 0x004912BB
    static bool getOrder(const SortMode mode, const openloco::station& lhs, const openloco::station& rhs)
    {
        switch (mode)
        {
            case SortMode::Name:
                return orderByName(lhs, rhs);

            case SortMode::Status:
            case SortMode::TotalUnitsWaiting:
                return orderByQuantity(lhs, rhs);

            case SortMode::CargoAccepted:
                return orderByAccepts(lhs, rhs);
        }

        return false;
    }

    // 0x0049111A
    static void updateStationList(window* window)
    {
        auto edi = -1;

        auto i = -1;

        for (auto& station : stationmgr::stations())
        {
            i++;
            if (station.empty())
                continue;

            if (station.owner != window->number)
                continue;

            if ((station.flags & station_flags::flag_5) != 0)
                continue;

            const uint16_t mask = tabInformationByType[window->current_tab].stationMask;
            if ((station.flags & mask) == 0)
                continue;

            if ((station.flags & station_flags::flag_4) != 0)
                continue;

            if (edi == -1)
            {
                edi = i;
                continue;
            }

            if (getOrder(SortMode(window->sort_mode), station, *stationmgr::get(edi)))
            {
                edi = i;
            }
        }

        if (edi != -1)
        {
            bool dl = false;

            stationmgr::get(edi)->flags |= station_flags::flag_4;

            auto ebp = window->row_count;
            if (edi != window->row_info[ebp])
            {
                window->row_info[ebp] = edi;
                dl = true;
            }

            window->row_count += 1;
            if (window->row_count > window->var_83C)
            {
                window->var_83C = window->row_count;
                dl = true;
            }

            if (dl)
            {
                window->invalidate();
            }
        }
        else
        {
            if (window->var_83C != window->row_count)
            {
                window->var_83C = window->row_count;
                window->invalidate();
            }

            refreshStationList(window);
        }
    }

    // 0x00490F6C
    window* open(company_id_t companyId)
    {
        window* window = WindowManager::bringToFront(WindowType::stationList, companyId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            // Still active?
            window = WindowManager::bringToFront(WindowType::stationList, companyId);
        }

        if (window == nullptr)
        {
            // 0x00491010
            window = WindowManager::createWindow(
                WindowType::stationList,
                window_size,
                window_flags::flag_11,
                &_events);

            window->number = companyId;
            window->owner = companyId;
            window->current_tab = 0;
            window->frame_no = 0;
            window->sort_mode = 0;
            window->var_83C = 0;
            window->row_hover = -1;

            refreshStationList(window);

            window->min_width = min_dimensions.width;
            window->min_height = min_dimensions.height;
            window->max_width = max_dimensions.width;
            window->max_height = max_dimensions.height;
            window->flags |= window_flags::resizable;

            auto interface = objectmgr::get<interface_skin_object>();
            window->colours[1] = interface->colour_0A;
        }

        // TODO: only needs to be called once.
        init_events();

        window->current_tab = 0;
        window->invalidate();

        window->widgets = (loco_ptr)_widgets;
        window->enabled_widgets = (1 << close_button) | (1 << tab_all_stations) | (1 << tab_rail_stations) | (1 << tab_road_stations) | (1 << tab_airports) | (1 << tab_ship_ports) | (1 << company_select) | (1 << sort_name) | (1 << sort_status) | (1 << sort_total_waiting) | (1 << sort_accepts) | (1 << scrollview);

        window->activated_widgets = 0;
        window->holdable_widgets = 0;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        return window;
    }

    window* open(company_id_t companyId, uint8_t type)
    {
        if (type > 4)
            throw std::domain_error("Unexpected station type");

        window* station_list = open(companyId);
        widx target = tabInformationByType[type].widgetIndex;
        station_list->call_on_mouse_up(target);

        return station_list;
    }

    // 0x004919A4
    static ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        if (widgetIdx != widx::scrollview)
            return fallback;

        uint16_t currentIndex = yPos / rowHeight;
        if (currentIndex < window->var_83C && window->row_info[currentIndex] != -1)
            return cursor_id::hand_pointer;

        return fallback;
    }

    // 0x0049196F
    static void event_08(window* window)
    {
        window->flags |= window_flags::flag_14;
    }

    // 0x00491977
    static void event_09(window* window)
    {
        if ((window->flags & window_flags::flag_14) == 0)
            return;

        if (window->row_hover == -1)
            return;

        window->row_hover = -1;
        window->invalidate();
    }

    // 0x00491344
    static void prepare_draw(ui::window* window)
    {
        // Reset active tab.
        window->activated_widgets &= ~((1 << tab_all_stations) | (1 << tab_rail_stations) | (1 << tab_road_stations) | (1 << tab_airports) | (1 << tab_ship_ports));
        window->activated_widgets |= (1ULL << tabInformationByType[window->current_tab].widgetIndex);

        // Set company name.
        auto company = companymgr::get(window->number);
        *_common_format_args = company->name;

        // Set window title.
        window->getWidget(widx::caption)->text = tabInformationByType[window->current_tab].windowTitleId;

        // Resize general window widgets.
        window->getWidget(widx::frame)->right = window->width - 1;
        window->getWidget(widx::frame)->bottom = window->height - 1;

        window->getWidget(widx::panel)->right = window->width - 1;
        window->getWidget(widx::panel)->bottom = window->height - 1;

        window->getWidget(widx::caption)->right = window->width - 2;

        window->getWidget(widx::close_button)->left = window->width - 15;
        window->getWidget(widx::close_button)->right = window->width - 3;

        window->getWidget(widx::scrollview)->right = window->width - 4;
        window->getWidget(widx::scrollview)->bottom = window->height - 14;

        // Reposition header buttons.
        window->getWidget(widx::sort_name)->right = std::min(203, window->width - 4);

        window->getWidget(widx::sort_status)->left = std::min(204, window->width - 4);
        window->getWidget(widx::sort_status)->right = std::min(403, window->width - 4);

        window->getWidget(widx::sort_total_waiting)->left = std::min(404, window->width - 4);
        window->getWidget(widx::sort_total_waiting)->right = std::min(493, window->width - 4);

        window->getWidget(widx::sort_accepts)->left = std::min(494, window->width - 4);
        window->getWidget(widx::sort_accepts)->right = std::min(613, window->width - 4);

        // Reposition company selection.
        window->getWidget(widx::company_select)->left = window->width - 28;
        window->getWidget(widx::company_select)->right = window->width - 3;

        // Set header button captions.
        window->getWidget(widx::sort_name)->text = window->sort_mode == SortMode::Name ? string_ids::table_header_name_desc : string_ids::table_header_name;
        window->getWidget(widx::sort_status)->text = window->sort_mode == SortMode::Status ? string_ids::table_header_status_desc : string_ids::table_header_status;
        window->getWidget(widx::sort_total_waiting)->text = window->sort_mode == SortMode::TotalUnitsWaiting ? string_ids::table_header_total_waiting_desc : string_ids::table_header_total_waiting;
        window->getWidget(widx::sort_accepts)->text = window->sort_mode == SortMode::CargoAccepted ? string_ids::table_header_accepts_desc : string_ids::table_header_accepts;

        // Reposition tabs (0x00491A39 / 0x00491A3F)
        int16_t new_tab_x = window->getWidget(widx::tab_all_stations)->left;
        int16_t tab_width = window->getWidget(widx::tab_all_stations)->right - new_tab_x;

        for (auto& tabInfo : tabInformationByType)
        {
            if (window->is_disabled(tabInfo.widgetIndex))
                continue;

            widget_t& tab =* window->getWidget(tabInfo.widgetIndex);

            tab.left = new_tab_x;
            new_tab_x += tab_width;
            tab.right = new_tab_x++;
        }
    }

    // 0x0049157F
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        auto shade = colour::get_shade(window->colours[1], 4);
        gfx::clear_single(*dpi, shade);

        uint16_t yPos = 0;
        for (uint16_t i = 0; i < window->var_83C; i++)
        {
            station_id_t stationId = window->row_info[i];

            // Skip items outside of view, or irrelevant to the current filter.
            if (yPos + rowHeight < dpi->y || yPos >= yPos + rowHeight + dpi->height || stationId == (uint16_t)-1)
            {
                yPos += rowHeight;
                continue;
            }

            string_id text_colour_id = string_ids::white_stringid2;

            // Highlight selection.
            if (stationId == window->row_hover)
            {
                gfx::draw_rect(dpi, 0, yPos, window->width, rowHeight, 0x2000030);
                text_colour_id = string_ids::wcolour2_stringid2;
            }

            auto station = stationmgr::get(stationId);

            // First, draw the town name.
            static const string_id label_icons[] = {
                string_ids::label_icons_none,
                string_ids::label_icons_rail,
                string_ids::label_icons_road,
                string_ids::label_icons_rail_road,
                string_ids::label_icons_air,
                string_ids::label_icons_rail_air,
                string_ids::label_icons_road_air,
                string_ids::label_icons_rail_road_air,
                string_ids::label_icons_water,
                string_ids::label_icons_rail_water,
                string_ids::label_icons_road_water,
                string_ids::label_icons_rail_road_water,
                string_ids::label_icons_air_water,
                string_ids::label_icons_rail_air_water,
                string_ids::label_icons_road_air_water,
                string_ids::label_icons_rail_road_air_water,
            };

            _common_format_args[0] = string_ids::stringid_stringid;
            _common_format_args[1] = station->name;
            _common_format_args[2] = station->town;
            _common_format_args[3] = label_icons[station->flags & 0x0F];

            gfx::draw_string_494BBF(*dpi, 0, yPos, 198, colour::black, text_colour_id, &*_common_format_args);

            // Then the station's current status.
            const char* buffer = stringmgr::get_string(string_ids::buffer_1250);
            station->getStatusString((char*)buffer);

            _common_format_args[0] = string_ids::buffer_1250;
            gfx::draw_string_494BBF(*dpi, 200, yPos, 198, colour::black, text_colour_id, &*_common_format_args);

            // Total units waiting.
            uint16_t totalUnits = 0;
            for (auto stats : station->cargo_stats)
                totalUnits += stats.quantity;

            _common_format_args[0] = string_ids::num_units;
            *(uint32_t*)&_common_format_args[1] = totalUnits;
            gfx::draw_string_494BBF(*dpi, 400, yPos, 88, colour::black, text_colour_id, &*_common_format_args);

            // And, finally, what goods the station accepts.
            char* ptr = (char*)buffer;
            *ptr = '\0';

            for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
            {
                auto& stats = station->cargo_stats[cargoId];

                if (!stats.is_accepted())
                    continue;

                if (*buffer != '\0')
                    ptr = stringmgr::format_string(ptr, string_ids::unit_separator);

                ptr = stringmgr::format_string(ptr, objectmgr::get<cargo_object>(cargoId)->name);
            }

            _common_format_args[0] = string_ids::buffer_1250;
            gfx::draw_string_494BBF(*dpi, 490, yPos, 118, colour::black, text_colour_id, &*_common_format_args);

            yPos += rowHeight;
        }
    }

    // 00491A76
    static void draw_tabs(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();
        auto companyColour = companymgr::get_company_colour(window->number);

        for (auto tab : tabInformationByType)
        {
            uint32_t image = gfx::recolour(skin->img + tab.imageId, companyColour);
            widget::draw_tab(window, dpi, image, tab.widgetIndex);
        }
    }

    // 0x004914D8
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets and tabs.
        window->draw(dpi);
        draw_tabs(window, dpi);

        // Draw company owner image.
        auto company = companymgr::get(window->number);
        auto competitor = objectmgr::get<competitor_object>(company->competitor_id);
        uint32_t image = gfx::recolour(competitor->images[company->owner_emotion], company->colour.primary);
        uint16_t x = window->x + window->getWidget(widx::company_select)->left + 1;
        uint16_t y = window->y + window->getWidget(widx::company_select)->top + 1;
        gfx::draw_image(dpi, x, y, image);

        // TODO: locale-based pluralisation.
        _common_format_args[0] = window->var_83C == 1 ? string_ids::status_num_stations_singular : string_ids::status_num_stations_plural;
        _common_format_args[1] = window->var_83C;

        // Draw number of stations.
        auto origin = gfx::point_t(window->x + 4, window->y + window->height - 12);
        gfx::draw_string_494B3F(*dpi, &origin, colour::black, string_ids::white_stringid2, &*_common_format_args);
    }

    // 0x004917BB
    static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::company_select)
            return;

        company_id_t companyId = dropdown::getCompanyIdFromSelection(itemIndex);

        // Try to find an open station list for this company.
        auto companyWindow = WindowManager::bringToFront(WindowType::stationList, companyId);
        if (companyWindow != nullptr)
            return;

        // If not, we'll turn this window into a window for the company selected.
        auto company = companymgr::get(companyId);
        if (company->name == string_ids::empty)
            return;

        window->number = companyId;
        window->owner = companyId;
        window->sort_mode = 0;
        window->row_count = 0;

        refreshStationList(window);

        window->var_83C = 0;
        window->row_hover = -1;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();
        window->invalidate();
    }

    // 0x004917B0
    static void on_mouse_down(ui::window* window, widget_index widgetIndex)
    {
        if (widgetIndex == widx::company_select)
            dropdown::populateCompanySelect(window, window->getWidget(widgetIndex));
    }

    // 0x00491785
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(window);
                break;

            case tab_all_stations:
            case tab_rail_stations:
            case tab_road_stations:
            case tab_airports:
            case tab_ship_ports:
            {
                if (input::is_tool_active(window->type, window->number))
                    input::cancel_tool();

                window->current_tab = widgetIndex - widx::tab_all_stations;
                window->frame_no = 0;

                window->invalidate();

                window->var_83C = 0;
                window->row_hover = -1;

                refreshStationList(window);

                window->call_on_resize();
                window->call_prepare_draw();
                window->init_scroll_widgets();
                window->moveInsideScreenEdges();
                break;
            }

            case sort_name:
            case sort_status:
            case sort_total_waiting:
            case sort_accepts:
            {
                auto sort_mode = widgetIndex - widx::sort_name;
                if (window->sort_mode == sort_mode)
                    return;

                window->sort_mode = sort_mode;
                window->invalidate();
                window->var_83C = 0;
                window->row_hover = -1;

                refreshStationList(window);
                break;
            }
        }
    }

    // 0x00491A0C
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentRow = y / rowHeight;
        if (currentRow > window->var_83C)
            return;

        int16_t currentStation = window->row_info[currentRow];
        if (currentStation == -1)
            return;

        windows::station::open(currentStation);
    }

    // 0x004919D1
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        window->flags &= ~(window_flags::flag_14);

        uint16_t currentRow = y / rowHeight;
        int16_t currentStation = -1;

        if (currentRow < window->var_83C)
            currentStation = window->row_info[currentRow];

        if (currentStation == window->row_hover)
            return;

        window->row_hover = currentStation;
        window->invalidate();
    }

    // 0x0049193F
    static void on_update(window* window)
    {
        window->frame_no++;

        window->call_prepare_draw();
        WindowManager::invalidateWidget(WindowType::stationList, window->number, window->current_tab + 4);

        // Add three stations every tick.
        updateStationList(window);
        updateStationList(window);
        updateStationList(window);
    }

    // 0x00491999
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = rowHeight * window->var_83C;
    }

    // 0x00491841
    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        *_common_format_args = string_ids::tooltip_scroll_station_list;
    }
}
