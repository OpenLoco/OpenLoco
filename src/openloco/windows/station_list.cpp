#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

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

    // static void call_8(window* window);
    // static void call_9(window* window);
    // static void cursor(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    // static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    // static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex);
    // static void on_mouse_down(ui::window* window, widget_index widgetIndex);
    // static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    // static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    // static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    // static void on_update(window* window);
    static void prepare_draw(ui::window* window);
    static void tooltip(ui::window* window, widget_index widgetIndex);

    static void init_events()
    {
        // _events.call_8 = call_8;
        // _events.call_9 = call_9;
        // _events.cursor = cursor;
        _events.draw = draw;
        // _events.draw_scroll = draw_scroll;
        _events.get_scroll_size = get_scroll_size;
        // _events.on_dropdown = on_dropdown;
        // _events.on_mouse_down = on_mouse_down;
        // _events.on_mouse_up = on_mouse_up;
        // _events.on_update = on_update;
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
        window* window = WindowManager::bringToFront(WindowType::stationList, 0);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            // Still active?
            window = WindowManager::bringToFront(WindowType::stationList, 0);
        }

        if (window == nullptr)
        {
            // 0x00491010
            window = WindowManager::createWindow(
                WindowType::stationList,
                window_size,
                window_flags::flag_19,
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
            window->flags = window_flags::stick_to_back | window_flags::scrolling_to_location;

            auto interface = objectmgr::get<interface_skin_object>();
            window->colours[0] = companymgr::get_company_colour(companyId);
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

        static widx type_to_widx[] = {
            tab_all_stations,
            tab_rail_stations,
            tab_road_stations,
            tab_airports,
            tab_ship_ports,
        };

        widx target = type_to_widx[type];
        station_list->call_on_mouse_up(target);

        return station_list;
    }

    // 0x00491344
    static void prepare_draw(ui::window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x00491344, regs);
    }

    // 0x004914D8
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // // Draw widgets.
        window->draw(dpi);

        registers regs;
        regs.esi = (int32_t)window;
        regs.edi = (int32_t)dpi;

        // This sub is only used in this drawing routine. Integrate.
        call(0x004914DD, regs);

        // Continue drawing with the original routine.
        call(0x004914E2, regs);
    }

    // 0x00491999
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = rowHeight * window->var_83C;
    }

    // 0x00491841
    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = string_ids::tooltip_scroll_station_list;
    }
}
