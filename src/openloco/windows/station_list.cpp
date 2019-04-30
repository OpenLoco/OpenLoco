#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::station_list
{
    static const gfx::ui_size_t window_size = { 600, 197 };
    static const gfx::ui_size_t max_dimensions = { 640, 1200 };
    static const gfx::ui_size_t min_dimensions = { 192, 100 };

    static const uint8_t rowHeight = 10;

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
        uint32_t imageId;
    };

    static TabDetails tabInformationByType[] = {
        { tab_all_stations, interface_skin::image_ids::all_stations },
        { tab_rail_stations, interface_skin::image_ids::rail_stations },
        { tab_road_stations, interface_skin::image_ids::road_stations },
        { tab_airports, interface_skin::image_ids::airports },
        { tab_ship_ports, interface_skin::image_ids::ship_ports }
    };

    loco_global<uint16_t[2], 0x112C826> _common_format_args;

    static void event_08(window* window);
    static void event_09(window* window);
    // static void cursor(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    // static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex);
    static void on_mouse_down(ui::window* window, widget_index widgetIndex);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    // static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    // static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_update(window* window);
    static void prepare_draw(ui::window* window);
    static void tooltip(ui::window* window, widget_index widgetIndex);

    static void init_events()
    {
        _events.event_08 = event_08;
        _events.event_09 = event_09;
        // _events.cursor = cursor;
        _events.draw = draw;
        // _events.draw_scroll = draw_scroll;
        _events.get_scroll_size = get_scroll_size;
        _events.on_dropdown = on_dropdown;
        _events.on_mouse_down = on_mouse_down;
        _events.on_mouse_up = on_mouse_up;
        _events.on_update = on_update;
        // _events.scroll_mouse_down = on_scroll_mouse_down;
        // _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.prepare_draw = prepare_draw;
        _events.tooltip = tooltip;
    }

    // 0x004910E8
    static void sub_4910E8(window* window)
    {
        // This sub is only called from events in this window.
        registers regs;
        regs.esi = (int32_t)window;
        call(0x004910E8, regs);
    }

    // 0x00490F6C
    window* open(uint16_t companyId)
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
            window->var_844 = 0;
            window->var_83C = 0;
            window->row_hover = -1;

            sub_4910E8(window);

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

        window->widgets = _widgets;
        window->enabled_widgets = (1 << close_button) | (1 << tab_all_stations) | (1 << tab_rail_stations) | (1 << tab_road_stations) | (1 << tab_airports) | (1 << tab_ship_ports) | (1 << company_select) | (1 << sort_name) | (1 << sort_status) | (1 << sort_total_waiting) | (1 << sort_accepts) | (1 << scrollview);

        window->activated_widgets = 0;
        window->holdable_widgets = 0;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        return window;
    }

    window* open(uint16_t companyId, uint8_t type)
    {
        if (type > 4)
            throw std::domain_error("Unexpected station type");

        window* station_list = open(companyId);
        widx target = tabInformationByType[type].widgetIndex;
        station_list->call_on_mouse_up(target);

        return station_list;
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

        if (window->row_hover == 0xFFFF)
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
        *_common_format_args = company->var_00;

        // Resize general window widgets.
        window->widgets[widx::frame].right = window->width - 1;
        window->widgets[widx::frame].bottom = window->height - 1;

        window->widgets[widx::panel].right = window->width - 1;
        window->widgets[widx::panel].bottom = window->height - 1;

        window->widgets[widx::caption].right = window->width - 2;

        window->widgets[widx::close_button].left = window->width - 15;
        window->widgets[widx::close_button].right = window->width - 3;

        window->widgets[widx::scrollview].right = window->width - 4;
        window->widgets[widx::scrollview].bottom = window->height - 14;

        // Reposition header buttons.
        window->widgets[widx::sort_name].right = std::min(203, window->width - 4);

        window->widgets[widx::sort_status].left = std::min(204, window->width - 4);
        window->widgets[widx::sort_status].right = std::min(403, window->width - 4);

        window->widgets[widx::sort_total_waiting].left = std::min(404, window->width - 4);
        window->widgets[widx::sort_total_waiting].right = std::min(493, window->width - 4);

        window->widgets[widx::sort_accepts].left = std::min(494, window->width - 4);
        window->widgets[widx::sort_accepts].right = std::min(613, window->width - 4);

        // Reposition company selection.
        window->widgets[widx::company_select].left = window->width - 28;
        window->widgets[widx::company_select].right = window->width - 3;

        // Set header button captions.
        window->widgets[widx::sort_name].text = window->var_844 == 0 ? string_ids::table_header_name_desc : string_ids::table_header_name;
        window->widgets[widx::sort_status].text = window->var_844 == 1 ? string_ids::table_header_status_desc : string_ids::table_header_status;
        window->widgets[widx::sort_total_waiting].text = window->var_844 == 2 ? string_ids::table_header_total_waiting_desc : string_ids::table_header_total_waiting;
        window->widgets[widx::sort_accepts].text = window->var_844 == 3 ? string_ids::table_header_accepts_desc : string_ids::table_header_accepts;

        // Reposition tabs (0x00491A39 / 0x00491A3F)
        widget_index tabs[] = { widx::tab_all_stations, widx::tab_rail_stations, widx::tab_road_stations, widx::tab_airports, widx::tab_ship_ports };
        int16_t new_tab_x = window->widgets[widx::tab_all_stations].left;
        int16_t tab_width = window->widgets[widx::tab_all_stations].right - new_tab_x;

        for (auto widgetIndex : tabs)
        {
            if (window->is_disabled(widgetIndex))
                continue;

            widget_t& tab = window->widgets[widgetIndex];

            tab.left = new_tab_x;
            new_tab_x += tab_width;
            tab.right = new_tab_x++;
        }
    }

    // 00491A76
    static void draw_tabs(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();
        auto companyColour = companymgr::get_company_colour(window->number);

        for (uint8_t stationType = 0; stationType < std::size(tabInformationByType); stationType++)
        {
            TabDetails tab = tabInformationByType[stationType];
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
        uint32_t image = gfx::recolour(competitor->images[company->var_19], company->colour.primary);
        uint16_t x = window->x + window->widgets[widx::company_select].left + 1;
        uint16_t y = window->y + window->widgets[widx::company_select].top + 1;
        gfx::draw_image(dpi, x, y, image);

        // TODO: locale-based pluralisation.
        _common_format_args[0] = window->var_83C == 1 ? string_ids::status_num_stations_singular : string_ids::status_num_stations_plural;
        _common_format_args[1] = window->var_83C;

        // Draw number of stations.
        gfx::point_t origin = { (int16_t)(window->x + 4), (int16_t)(window->y + window->height - 12) };
        gfx::draw_string_494B3F(*dpi, &origin, colour::black, string_ids::white_stringid2, &*_common_format_args);
    }

    // 0x004917BB
    static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex == widx::company_select)
        {
            registers regs;
            regs.edx = (int32_t)widgetIndex;
            regs.esi = (int32_t)window;
            regs.ax = itemIndex;

            call(0x004917C2);
        }
    }

    // 0x004917B0
    static void on_mouse_down(ui::window* window, widget_index widgetIndex)
    {
        if (widgetIndex == widx::company_select)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.edi = (int32_t)&window->widgets[widgetIndex];
            regs.esi = (int32_t)window;

            call(0x004CF2B3, regs);
        }
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

                sub_4910E8(window);

                window->call_on_resize();
                window->call_prepare_draw();
                window->init_scroll_widgets();

                registers regs;
                regs.esi = (int32_t)window;
                regs.edx = widgetIndex;
                call(0x004CD320, regs);
                break;
            }

            case sort_name:
            case sort_status:
            case sort_total_waiting:
            case sort_accepts:
            {
                auto sort_mode = widgetIndex - widx::sort_name;
                if (window->var_844 == sort_mode)
                    return;

                window->var_844 = sort_mode;
                window->invalidate();
                window->var_83C = 0;
                window->row_hover = -1;

                call(0x004910E8, regs);
                break;
            }
        }
    }

    // 0x0049193F
    static void on_update(window* window)
    {
        window->frame_no++;

        window->call_prepare_draw();
        WindowManager::invalidate(WindowType::stationList, window->number);

        registers regs;
        regs.esi = (int32_t)window;

        // Why is this called three times??
        call(0x0049111A, regs);
        call(0x0049111A, regs);
        call(0x0049111A, regs);
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
