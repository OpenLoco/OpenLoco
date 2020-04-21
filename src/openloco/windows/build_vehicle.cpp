#include "../companymgr.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/cargo_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_extra_object.h"
#include "../objects/road_object.h"
#include "../objects/track_extra_object.h"
#include "../objects/track_object.h"
#include "../objects/vehicle_object.h"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::build_vehicle
{
    static const gfx::ui_size_t window_size = { 380, 233 };

    enum widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_build_new_trains,
        tab_build_new_buses,
        tab_build_new_trucks,
        tab_build_new_trams,
        tab_build_new_aircraft,
        tab_build_new_ships,
        tab_vehicles_for_0,
        tab_vehicles_for_1,
        tab_vehicles_for_2,
        tab_vehicles_for_3,
        tab_vehicles_for_4,
        tab_vehicles_for_5,
        tab_vehicles_for_6,
        tab_vehicles_for_7,
        scrollview_vehicle_selection,
        scrollview_vehicle_preview
    };

    enum scrollIdx
    {
        vehicle_selection,
        vehicle_preview
    };

    struct TabDetails
    {
        widx widgetIndex;
        uint32_t imageId;
    };

    static TabDetails typeTabInformationByType[] = {
        { tab_build_new_trains, interface_skin::image_ids::build_vehicle_train },
        { tab_build_new_buses, interface_skin::image_ids::build_vehicle_bus },
        { tab_build_new_trucks, interface_skin::image_ids::build_vehicle_truck },
        { tab_build_new_trams, interface_skin::image_ids::build_vehicle_tram },
        { tab_build_new_aircraft, interface_skin::image_ids::build_vehicle_aircraft },
        { tab_build_new_ships, interface_skin::image_ids::build_vehicle_ship }
    };

    // 0x5231D0
    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 380, 233 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 378, 13 }, widget_type::caption_24, 0),
        make_widget({ 365, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 380, 192 }, widget_type::panel, 1),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_train_vehicles),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_buses),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_trucks),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_trams),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_aircraft),
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_ships),
        make_remap_widget({ 5, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 36, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 67, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 98, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 129, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 160, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 191, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_remap_widget({ 222, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        make_widget({ 3, 72 }, { 374, 146 }, widget_type::scrollview, 1, scrollbars::vertical),
        make_widget({ 250, 44 }, { 180, 66 }, widget_type::scrollview, 1, scrollbars::none),
        widget_end(),
    };

    static constexpr uint32_t widget_index_to_tab_vehicle_for(widget_index widgetIndex)
    {
        return widgetIndex - widx::tab_vehicles_for_0;
    }

    loco_global<uint16_t[8], 0x112C826> _common_format_args;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;
    static loco_global<int16_t, 0x01136268> _num_available_vehicles;
    static loco_global<uint16_t[objectmgr::get_max_objects(object_type::vehicle)], 0x0113626A> _available_vehicles;
    static loco_global<uint16_t, 0x0113642A> _113642A;
    static loco_global<int32_t, 0x011364E8> _build_target_vehicle;
    static loco_global<uint32_t, 0x011364EC> _num_track_type_tabs;
    // Array of types if 0xFF then no type, flag (1<<7) as well
    static loco_global<int8_t[widget_index_to_tab_vehicle_for(widx::tab_vehicles_for_7) + 1], 0x011364F0> _tab_track_types;
    static loco_global<uint32_t[32], 0x00525E5E> currencyMultiplicationFactor;
    static loco_global<uint8_t, 0x00525FC5> _525FC5;
    static std::array<uint16_t, 6> _scroll_row_height = { { 22, 22, 22, 22, 42, 30 } };
    static loco_global<uint8_t, 0x00525FAA> last_railroad_option;
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x0052622C> last_build_vehicles_option;
    static loco_global<uint16_t, 0x0052622E> _52622E; // Tick related

    static window_event_list _events;

    static void sub_4B92A5(ui::window* window);
    static void SetDisabledTransportTabs(ui::window* window);
    static void SetTrackTypeTabs(ui::window* window);
    static void ResetTrackTypeTabSelection(ui::window* window);
    static void SetTopToolbarLastTrack(uint8_t track_type, bool is_road);
    static void SetTransportTypeTabs(ui::window* window);
    static void sub_4B60CC(openloco::vehicle* vehicle);
    static void draw_vehicle_overview(gfx::drawpixelinfo_t* dpi, int16_t vehicle_type_idx, company_id_t company, uint8_t eax, uint8_t esi, gfx::point_t offset);
    static int16_t draw_vehicle_inline(gfx::drawpixelinfo_t* dpi, int16_t vehicle_type_idx, uint8_t unk_1, company_id_t company, gfx::point_t loc);
    static void draw_build_tabs(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_build_sub_type_tabs(ui::window* window, gfx::drawpixelinfo_t* dpi);

    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_resize(window* window);
    static void on_periodic_update(window* window);
    static void on_update(window* window);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex);
    static ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void prepare_draw(ui::window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);

    static void init_events()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.on_resize = on_resize;
        _events.on_periodic_update = on_periodic_update;
        _events.on_update = on_update;
        _events.get_scroll_size = get_scroll_size;
        _events.scroll_mouse_down = on_scroll_mouse_down;
        _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.tooltip = tooltip;
        _events.cursor = cursor;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;
    }

    // 0x4C1C64
    static window* create(company_id_t company)
    {
        init_events();
        auto window = WindowManager::createWindow(WindowType::buildVehicle, window_size, window_flags::flag_11, &_events);
        window->widgets = _widgets;
        window->number = company;
        window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_vehicles_for_0) | (1 << widx::tab_vehicles_for_1) | (1 << widx::tab_vehicles_for_2) | (1 << widx::tab_vehicles_for_3) | (1 << widx::tab_vehicles_for_4) | (1 << widx::tab_vehicles_for_5) | (1 << widx::tab_vehicles_for_6) | (1 << widx::tab_vehicles_for_7) | (1 << widx::scrollview_vehicle_selection);
        window->owner = companymgr::get_controlling_id();
        window->frame_no = 0;
        auto skin = openloco::objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[1] = skin->colour_0A;
        }
        SetDisabledTransportTabs(window);
        return window;
    }

    /* 0x4C1AF7
     * depending on flags (1<<31) vehicle is a tab id or a vehicle thing_id
     */
    window* open(uint32_t vehicle, uint32_t flags)
    {
        auto window = WindowManager::bringToFront(WindowType::buildVehicle, companymgr::get_controlling_id());
        bool tabMode = flags & (1 << 31);
        if (window)
        {
            widget_index tab = widx::tab_build_new_trains;
            if (!tabMode)
            {
                auto veh = thingmgr::get<openloco::vehicle>(vehicle);
                tab += veh->var_5E;
            }
            else
            {
                // Not a vehicle but a type
                tab += vehicle;
            }
            window->call_on_mouse_up(tab);

            if (tabMode)
            {
                _build_target_vehicle = -1;
            }
            else
            {
                _build_target_vehicle = vehicle;
            }
        }
        else
        {
            window = create(companymgr::get_controlling_id());
            window->width = window_size.width;
            window->height = window_size.height;
            _build_target_vehicle = -1;
            if (!tabMode)
            {
                _build_target_vehicle = vehicle;
                auto veh = thingmgr::get<openloco::vehicle>(vehicle);
                window->current_tab = veh->var_5E;
            }
            else
            {
                window->current_tab = vehicle;
            }

            window->row_height = _scroll_row_height[window->current_tab];
            window->row_count = 0;
            window->var_83C = 0;
            window->row_hover = -1;
            window->invalidate();
            window->widgets = _widgets;
            window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_vehicles_for_0) | (1 << widx::tab_vehicles_for_1) | (1 << widx::tab_vehicles_for_2) | (1 << widx::tab_vehicles_for_3) | (1 << widx::tab_vehicles_for_4) | (1 << widx::tab_vehicles_for_5) | (1 << widx::tab_vehicles_for_6) | (1 << widx::tab_vehicles_for_7) | (1 << widx::scrollview_vehicle_selection);
            window->holdable_widgets = 0;
            window->event_handlers = &_events;
            window->activated_widgets = 0;
            SetDisabledTransportTabs(window);
            SetTrackTypeTabs(window);
            ResetTrackTypeTabSelection(window);
            sub_4B92A5(window);

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
        }

        if (_build_target_vehicle == -1)
        {
            return window;
        }
        auto veh = thingmgr::get<openloco::vehicle>(_build_target_vehicle);
        if (veh == nullptr)
        {
            return window;
        }
        auto target_track_type = veh->track_type;
        if (veh->mode != TransportMode::rail)
        {
            target_track_type |= (1 << 7);
            if (target_track_type == 0xFF)
            {
                target_track_type = _525FC5;
            }
        }

        widget_index widgetIndex = widx::tab_vehicles_for_0;
        for (uint32_t track_tab = 0; track_tab < _num_track_type_tabs; track_tab++)
        {
            if (target_track_type == _tab_track_types[track_tab])
            {
                widgetIndex = widx::tab_vehicles_for_0 + track_tab;
                break;
            }
        }

        window->call_on_mouse_up(widgetIndex);
        return window;
    }

    void registerHooks()
    {
        register_hook(
            0x004B92A5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4B92A5((ui::window*)regs.esi);
                regs = backup;
                return 0;
            });

        register_hook(
            0x4C1AF7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                open(regs.eax, regs.eax);
                regs = backup;
                return 0;
            });
    }

    static bool sub_4B8FA2(openloco::vehicle* esi, uint32_t eax)
    {
        registers regs;
        regs.eax = eax;
        regs.esi = (uintptr_t)esi;
        if (esi == nullptr)
        {
            regs.esi = -1;
        }

        return call(0x004B8FA2, regs) & (X86_FLAG_CARRY << 8);
    }

    /* 0x4B9165
     * Works out which vehicles are able to be built for this vehicle_type or vehicle
     */
    static void GenerateBuildableVehiclesArray(VehicleType vehicle_type, uint8_t track_type, openloco::vehicle* vehicle)
    {
        if (track_type != 0xFF && (track_type & (1 << 7)))
        {
            auto obj_idx = track_type & ~(1 << 7);
            auto road_obj = objectmgr::get<road_object>(obj_idx);
            if (road_obj->flags & flags_12::unk_03)
            {
                track_type = 0xFE;
            }
        }

        auto company_id = companymgr::get_controlling_id();
        if (vehicle != nullptr)
        {
            company_id = vehicle->owner;
        }
        _num_available_vehicles = 0;
        struct build_item
        {
            uint16_t vehicle_index;
            uint8_t power;
            uint16_t designed;
        };
        std::vector<build_item> buildable_vehicles;

        for (uint16_t vehicle_obj_index = 0; vehicle_obj_index < objectmgr::get_max_objects(object_type::vehicle); ++vehicle_obj_index)
        {
            auto vehicle_obj = objectmgr::get<vehicle_object>(vehicle_obj_index);
            if ((uint32_t)vehicle_obj == 0xFFFFFFFF)
            {
                continue;
            }

            if (vehicle)
            {
                if (sub_4B8FA2(vehicle, vehicle_obj_index))
                {
                    continue;
                }
            }

            if (vehicle_obj->type != vehicle_type)
            {
                continue;
            }

            // Is vehicle type unlocked
            if (!(companymgr::get(company_id)->unlocked_vehicles[vehicle_obj_index >> 5] & (1 << (vehicle_obj_index & 0x1F))))
            {
                continue;
            }

            if (track_type != 0xFF)
            {
                uint8_t var05 = track_type;
                if (track_type & (1 << 7))
                {
                    if (vehicle_obj->mode != TransportMode::road)
                    {
                        continue;
                    }

                    if (track_type == 0xFE)
                    {
                        var05 = 0xFF;
                    }
                    else
                    {
                        var05 = track_type & ~(1 << 7);
                    }
                }
                else
                {
                    if (vehicle_obj->mode != TransportMode::rail)
                    {
                        continue;
                    }
                }

                if (var05 != vehicle_obj->track_type)
                {
                    continue;
                }
            }

            auto power = std::min<uint16_t>(vehicle_obj->power, 1);
            // Unsure why power is only checked for first byte.
            buildable_vehicles.push_back({ vehicle_obj_index, static_cast<uint8_t>(power), vehicle_obj->designed });
        }

        std::sort(buildable_vehicles.begin(), buildable_vehicles.end(), [](const build_item& item1, const build_item& item2) { return item1.designed < item2.designed; });
        std::sort(buildable_vehicles.begin(), buildable_vehicles.end(), [](const build_item& item1, const build_item& item2) { return item1.power > item2.power; });
        for (size_t i = 0; i < buildable_vehicles.size(); ++i)
        {
            _available_vehicles[i] = buildable_vehicles[i].vehicle_index;
        }
        _num_available_vehicles = static_cast<int16_t>(buildable_vehicles.size());
    }

    static ui::window* getTopEditingVehicleWindow()
    {
        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            if (w->current_tab != 1)
                continue;

            auto vehicle = thingmgr::get<openloco::vehicle>(w->number);
            if (vehicle->owner != companymgr::get_controlling_id())
                continue;

            return w;
        }

        return nullptr;
    }

    /**
     * 0x004B92A5
     *
     * @param window @<esi>
     */
    static void sub_4B92A5(ui::window* window)
    {
        auto w = getTopEditingVehicleWindow();
        int32_t vehicleId = -1;
        if (w != nullptr)
        {
            vehicleId = w->number;
        }

        if (_build_target_vehicle != vehicleId)
        {
            _build_target_vehicle = vehicleId;
            window->var_83C = 0;
            window->invalidate();
        }

        VehicleType vehicleType = static_cast<VehicleType>(window->current_tab);
        uint8_t track_type = _tab_track_types[window->current_secondary_tab];

        openloco::vehicle* veh = nullptr;
        if (_build_target_vehicle != -1)
        {
            veh = thingmgr::get<openloco::vehicle>(_build_target_vehicle);
        }

        GenerateBuildableVehiclesArray(vehicleType, track_type, veh);

        int num_rows = _num_available_vehicles;
        if (window->var_83C == num_rows)
            return;

        uint16_t* src = _available_vehicles;
        int16_t* dest = window->row_info;
        window->var_83C = num_rows;
        window->row_count = 0;
        while (num_rows != 0)
        {
            *dest = *src;
            dest++;
            src++;
            num_rows--;
        }
        window->row_hover = -1;
        window->invalidate();
    }

    // 0x4C3576
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(window);
                break;

            case widx::tab_build_new_trains:
            case widx::tab_build_new_buses:
            case widx::tab_build_new_trucks:
            case widx::tab_build_new_trams:
            case widx::tab_build_new_aircraft:
            case widx::tab_build_new_ships:
            {

                if (input::has_flag(input::input_flags::tool_active))
                {
                    input::cancel_tool(window->type, window->number);
                }

                auto new_tab = widgetIndex - widx::tab_build_new_trains;
                window->current_tab = new_tab;
                window->row_height = _scroll_row_height[new_tab];
                window->frame_no = 0;
                window->current_secondary_tab = 0;
                if (new_tab != last_build_vehicles_option)
                {
                    last_build_vehicles_option = new_tab;
                    WindowManager::invalidate(WindowType::topToolbar, 0);
                }

                auto cur_viewport = window->viewports[0];
                window->viewports[0] = 0;
                if (cur_viewport != 0)
                {
                    cur_viewport->width = 0;
                }

                window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_vehicles_for_0) | (1 << widx::tab_vehicles_for_1) | (1 << widx::tab_vehicles_for_2) | (1 << widx::tab_vehicles_for_3) | (1 << widx::tab_vehicles_for_4) | (1 << widx::tab_vehicles_for_5) | (1 << widx::tab_vehicles_for_6) | (1 << widx::tab_vehicles_for_7) | (1 << widx::scrollview_vehicle_selection);
                window->holdable_widgets = 0;
                window->event_handlers = &_events;
                window->widgets = _widgets;
                SetDisabledTransportTabs(window);
                window->invalidate();
                _build_target_vehicle = -1;
                SetTrackTypeTabs(window);
                ResetTrackTypeTabSelection(window);
                window->row_count = 0;
                window->var_83C = 0;
                window->row_hover = -1;
                window->call_on_resize();
                window->call_on_periodic_update();
                window->call_prepare_draw();
                window->init_scroll_widgets();
                window->invalidate();
                window->moveInsideScreenEdges();
                break;
            }
            case widx::tab_vehicles_for_0:
            case widx::tab_vehicles_for_1:
            case widx::tab_vehicles_for_2:
            case widx::tab_vehicles_for_3:
            case widx::tab_vehicles_for_4:
            case widx::tab_vehicles_for_5:
            case widx::tab_vehicles_for_6:
            case widx::tab_vehicles_for_7:
            {
                auto tab = widget_index_to_tab_vehicle_for(widgetIndex);
                if (window->current_secondary_tab == tab)
                    break;

                window->current_secondary_tab = tab;
                SetTopToolbarLastTrack(_tab_track_types[tab] & ~(1 << 7), _tab_track_types[tab] & (1 << 7));
                _build_target_vehicle = -1;
                window->row_count = 0;
                window->var_83C = 0;
                window->row_hover = -1;
                window->call_on_resize();
                window->call_on_periodic_update();
                window->call_prepare_draw();
                window->init_scroll_widgets();
                window->invalidate();
                break;
            }
        }
    }

    // 0x4C3929
    static void on_resize(window* window)
    {
        window->flags |= window_flags::resizable;
        auto min_width = std::max<int16_t>(_num_track_type_tabs * 31 + 195, 380);
        window->min_width = min_width;
        window->max_width = 520;
        window->min_height = 233;
        window->max_height = 600;
        if (window->width < min_width)
        {
            window->width = min_width;
            window->invalidate();
        }

        if (window->height < window->min_height)
        {
            window->height = window->min_height;
            window->invalidate();
        }

        auto scroll_position = window->scroll_areas[scrollIdx::vehicle_selection].v_bottom;
        scroll_position -= window->widgets[widx::scrollview_vehicle_selection].bottom;
        scroll_position += window->widgets[widx::scrollview_vehicle_selection].top;
        if (scroll_position < 0)
        {
            scroll_position = 0;
        }

        if (scroll_position < window->scroll_areas[scrollIdx::vehicle_selection].v_top)
        {
            window->scroll_areas[scrollIdx::vehicle_selection].v_top = scroll_position;
            ui::scrollview::update_thumbs(window, widx::scrollview_vehicle_selection);
        }

        if (window->row_hover != -1)
        {
            return;
        }

        if (window->var_83C == 0)
        {
            return;
        }

        window->row_hover = window->row_info[0];
        window->invalidate();
    }

    // 0x4C3923
    static void on_periodic_update(window* window)
    {
        sub_4B92A5(window);
    }

    // 0x4C377B
    static void on_update(window* window)
    {
        window->frame_no++;
        window->call_prepare_draw();

        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, window->current_tab + 4);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, (window->current_secondary_tab & 0xFF) + 10);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, 19);
    }

    // 0x4C37B9
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->var_83C * window->row_height;
    }

    // 0x4C384B
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scroll_item = y / window->row_height;
        if (scroll_item >= window->var_83C)
        {
            return;
        }

        auto pan = window->width / 2 + window->x;
        audio::play_sound(audio::sound_id::click_down, loc16{ x, y, static_cast<int16_t>(pan) }, pan);
        auto item = window->row_info[scroll_item];
        auto vehicle_obj = objectmgr::get<vehicle_object>(item);
        _common_format_args[5] = vehicle_obj->name;
        gGameCommandErrorTitle = string_ids::cant_build_pop_5_string_id;
        if (_build_target_vehicle != -1)
        {
            auto vehicle = thingmgr::get<openloco::vehicle>(_build_target_vehicle);
            _common_format_args[6] = vehicle->var_44;
            _common_format_args[7] = vehicle->var_22;
            gGameCommandErrorTitle = string_ids::cant_add_pop_5_string_id_string_id;
        }

        if (!game_commands::do_5(item, _build_target_vehicle))
        {
            return;
        }

        if (_build_target_vehicle == -1)
        {
            auto vehicle = thingmgr::get<openloco::vehicle>(_113642A);
            sub_4B60CC(vehicle);
        }
        sub_4B92A5(window);
    }

    // 0x4C3802
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scroll_item = y / window->row_height;
        int16_t item = -1;
        if (scroll_item < window->var_83C)
        {
            item = window->row_info[scroll_item];
        }

        if (item != -1 && item != window->row_hover)
        {
            window->row_hover = item;
            window->invalidate();
        }
    }

    // 0x4C370C
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
    {
        if (widgetIndex < widx::tab_vehicles_for_0 || widgetIndex >= widx::scrollview_vehicle_selection)
        {
            args.push(string_ids::tooltip_scroll_new_vehicle_list);
        }
        else
        {
            auto track_tab = widget_index_to_tab_vehicle_for(widgetIndex);
            auto type = _tab_track_types[track_tab];
            if (type == -1)
            {
                if (window->current_tab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains))
                {

                    args.push(string_ids::airport);
                }
                else
                {
                    args.push(string_ids::docks);
                }
            }
            else
            {
                bool is_road = type & (1 << 7);
                type &= ~(1 << 7);
                if (is_road)
                {
                    auto road_obj = objectmgr::get<road_object>(type);
                    args.push(road_obj->name);
                }
                else
                {
                    auto track_obj = objectmgr::get<track_object>(type);
                    args.push(track_obj->name);
                }
            }
        }
    }

    // 0x4C37CB
    static ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        if (widgetIdx != widx::scrollview_vehicle_selection)
        {
            return fallback;
        }

        auto scroll_item = yPos / window->row_height;
        if (scroll_item >= window->var_83C)
        {
            return fallback;
        }

        if (window->row_info[scroll_item] == -1)
        {
            return fallback;
        }

        return cursor_id::hand_pointer;
    }

    // 0x4C2E5C
    static void prepare_draw(ui::window* window)
    {
        if (window->widgets != _widgets)
        {
            window->widgets = _widgets;
            window->init_scroll_widgets();
        }

        // Mask off all the tabs
        auto active_widgets = window->activated_widgets & ((1 << frame) | (1 << caption) | (1 << close_button) | (1 << panel) | (1 << scrollview_vehicle_selection) | (1 << scrollview_vehicle_preview));
        // Only activate the singular tabs
        active_widgets |= 1ULL << (window->current_tab + widx::tab_build_new_trains);
        active_widgets |= 1ULL << (window->current_secondary_tab + widx::tab_vehicles_for_0);
        window->activated_widgets = active_widgets;

        window->widgets[widx::caption].text = window->current_tab + string_ids::build_trains;

        auto width = window->width;
        auto height = window->height;

        window->widgets[widx::frame].right = width - 1;
        window->widgets[widx::frame].bottom = height - 1;

        window->widgets[widx::panel].right = width - 1;
        window->widgets[widx::panel].bottom = height - 1;

        window->widgets[widx::caption].right = width - 2;

        window->widgets[widx::close_button].left = width - 15;
        window->widgets[widx::close_button].right = width - 3;

        window->widgets[widx::scrollview_vehicle_preview].right = width - 4;
        window->widgets[widx::scrollview_vehicle_preview].left = width - 184;

        window->widgets[widx::scrollview_vehicle_selection].right = width - 187;
        window->widgets[widx::scrollview_vehicle_selection].bottom = height - 14;

        SetTransportTypeTabs(window);
    }

    // 0x4C2F23
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        window->draw(dpi);
        draw_build_tabs(window, dpi);
        draw_build_sub_type_tabs(window, dpi);

        {
            auto x = window->x + 2;
            auto y = window->y + window->height - 13;
            auto bottom_left_message = string_ids::select_new_vehicle;
            if (_build_target_vehicle != -1)
            {
                auto vehicle = thingmgr::get<openloco::vehicle>(_build_target_vehicle);
                _common_format_args[0] = vehicle->var_22;
                _common_format_args[1] = vehicle->var_44;
                bottom_left_message = string_ids::select_vehicle_to_add_to_string_id;
            }

            gfx::draw_string_494BBF(*dpi, x, y, window->width - 186, colour::black, bottom_left_message, _common_format_args);
        }

        if (window->row_hover == -1)
        {
            return;
        }

        auto vehicle_obj = objectmgr::get<vehicle_object>(window->row_hover);
        auto buffer = const_cast<char*>(stringmgr::get_string(string_ids::buffer_1250));
        auto cost = (vehicle_obj->cost_fact * currencyMultiplicationFactor[vehicle_obj->cost_ind]) / 64;
        *(reinterpret_cast<uint32_t*>(&_common_format_args[0])) = cost;
        buffer = stringmgr::format_string(buffer, string_ids::stats_cost, _common_format_args);

        auto running_cost = (vehicle_obj->run_cost_fact * currencyMultiplicationFactor[vehicle_obj->run_cost_ind]) / 1024;
        *(reinterpret_cast<uint32_t*>(&_common_format_args[0])) = running_cost;
        buffer = stringmgr::format_string(buffer, string_ids::stats_running_cost, _common_format_args);

        if (vehicle_obj->designed != 0)
        {
            _common_format_args[0] = vehicle_obj->designed;
            buffer = stringmgr::format_string(buffer, string_ids::stats_designed, _common_format_args);
        }

        if (vehicle_obj->mode == TransportMode::rail || vehicle_obj->mode == TransportMode::road)
        {
            buffer = stringmgr::format_string(buffer, string_ids::stats_requires);
            auto track_name = string_ids::road;
            if (vehicle_obj->mode == TransportMode::road)
            {
                if (vehicle_obj->track_type != 0xFF)
                {
                    track_name = objectmgr::get<road_object>(vehicle_obj->track_type)->name;
                }
            }
            else
            {
                track_name = objectmgr::get<track_object>(vehicle_obj->track_type)->name;
            }

            buffer = stringmgr::format_string(buffer, track_name);

            for (auto i = 0; i < vehicle_obj->num_mods; ++i)
            {
                strcpy(buffer, " + ");
                buffer += 3;
                if (vehicle_obj->mode == TransportMode::road)
                {
                    auto road_extra_obj = objectmgr::get<road_extra_object>(vehicle_obj->required_track_extras[i]);
                    buffer = stringmgr::format_string(buffer, road_extra_obj->name);
                }
                else
                {
                    auto track_extra_obj = objectmgr::get<track_extra_object>(vehicle_obj->required_track_extras[i]);
                    buffer = stringmgr::format_string(buffer, track_extra_obj->name);
                }
            }

            if (vehicle_obj->flags & flags_E0::rack_rail)
            {
                auto track_extra_obj = objectmgr::get<track_extra_object>(vehicle_obj->rack_rail_type);
                _common_format_args[0] = track_extra_obj->name;
                buffer = stringmgr::format_string(buffer, string_ids::stats_string_steep_slope, _common_format_args);
            }
        }

        if (vehicle_obj->power != 0)
        {
            if (vehicle_obj->mode == TransportMode::rail || vehicle_obj->mode == TransportMode::road)
            {
                _common_format_args[0] = vehicle_obj->power;
                buffer = stringmgr::format_string(buffer, string_ids::stats_power, _common_format_args);
            }
        }

        _common_format_args[0] = vehicle_obj->weight;
        buffer = stringmgr::format_string(buffer, string_ids::stats_weight, _common_format_args);
        _common_format_args[0] = vehicle_obj->speed;
        buffer = stringmgr::format_string(buffer, string_ids::stats_max_speed, _common_format_args);

        if (vehicle_obj->flags & flags_E0::rack_rail)
        {
            auto track_extra_obj = objectmgr::get<track_extra_object>(vehicle_obj->rack_rail_type);
            _common_format_args[0] = vehicle_obj->rack_speed;
            _common_format_args[1] = track_extra_obj->name;
            buffer = stringmgr::format_string(buffer, string_ids::stats_velocity_on_string, _common_format_args);
        }

        if (vehicle_obj->num_simultaneous_cargo_types != 0)
        {
            {
                *(reinterpret_cast<uint32_t*>(&_common_format_args[1])) = vehicle_obj->max_primary_cargo;
                auto cargo_type = utility::bitscanforward(vehicle_obj->primary_cargo_types);
                if (cargo_type != -1)
                {
                    auto cargo_types = vehicle_obj->primary_cargo_types & ~(1 << cargo_type);
                    auto cargo_obj = objectmgr::get<cargo_object>(cargo_type);
                    _common_format_args[0] = vehicle_obj->max_primary_cargo == 1 ? cargo_obj->unit_name_singular : cargo_obj->unit_name_plural;
                    buffer = stringmgr::format_string(buffer, string_ids::stats_capacity, _common_format_args);
                    cargo_type = utility::bitscanforward(cargo_types);
                    if (cargo_type != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargo_type != -1; cargo_type = utility::bitscanforward(cargo_types))
                        {
                            cargo_types &= ~(1 << cargo_type);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            cargo_obj = objectmgr::get<cargo_object>(cargo_type);
                            _common_format_args[0] = cargo_obj->name;
                            buffer = stringmgr::format_string(buffer, string_ids::stats_or_string, _common_format_args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }

            if (vehicle_obj->flags & flags_E0::refittable)
            {
                buffer = stringmgr::format_string(buffer, string_ids::stats_refittable);
            }

            if (vehicle_obj->num_simultaneous_cargo_types > 1)
            {
                *(reinterpret_cast<uint32_t*>(&_common_format_args[1])) = vehicle_obj->max_secondary_cargo;
                auto cargo_type = utility::bitscanforward(vehicle_obj->secondary_cargo_types);
                if (cargo_type != -1)
                {
                    auto cargo_types = vehicle_obj->secondary_cargo_types & ~(1 << cargo_type);
                    auto cargo_obj = objectmgr::get<cargo_object>(cargo_type);
                    _common_format_args[0] = vehicle_obj->max_primary_cargo == 1 ? cargo_obj->unit_name_singular : cargo_obj->unit_name_plural;
                    buffer = stringmgr::format_string(buffer, string_ids::stats_plus_string, _common_format_args);

                    cargo_type = utility::bitscanforward(cargo_types);
                    if (cargo_type != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargo_type != -1; cargo_type = utility::bitscanforward(cargo_types))
                        {
                            cargo_types &= ~(1 << cargo_type);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            cargo_obj = objectmgr::get<cargo_object>(cargo_type);
                            _common_format_args[0] = cargo_obj->name;
                            buffer = stringmgr::format_string(buffer, string_ids::stats_or_string, _common_format_args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }
        }

        auto x = window->widgets[widx::scrollview_vehicle_selection].right + window->x + 2;
        auto y = window->widgets[widx::scrollview_vehicle_preview].bottom + window->y + 2;
        gfx::draw_string_495224(*dpi, x, y, 180, colour::black, string_ids::buffer_1250);
    }

    // 0x4C3307
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        switch (scrollIndex)
        {
            case scrollIdx::vehicle_selection:
            {
                auto colour = colour::get_shade(window->colours[1], 4);
                gfx::clear(*dpi, colour * 0x01010101);
                if (window->var_83C == 0)
                {
                    auto default_message = string_ids::no_vehicles_available;
                    if (_build_target_vehicle != -1)
                    {
                        auto vehicle = thingmgr::get<openloco::vehicle>(_build_target_vehicle);
                        default_message = string_ids::no_compatible_vehicles_available;
                        _common_format_args[0] = vehicle->var_22;
                        _common_format_args[1] = vehicle->var_44;
                    }

                    auto width = window->widgets[widx::scrollview_vehicle_selection].right - window->widgets[widx::scrollview_vehicle_selection].left - 17;
                    auto y = (window->row_height - 10) / 2;
                    gfx::draw_string_495224(*dpi, 3, y, width, colour::black, default_message, _common_format_args);
                }
                else
                {
                    int16_t y = 0;
                    for (auto i = 0; i < window->var_83C; ++i, y += window->row_height)
                    {
                        if (y + window->row_height + 30 <= dpi->y)
                        {
                            continue;
                        }

                        if (y >= dpi->y + dpi->height + 30)
                        {
                            continue;
                        }

                        auto vehicle_type = window->row_info[i];
                        if (vehicle_type == -1)
                        {
                            continue;
                        }

                        auto coloured_string = string_ids::white_stringid2;
                        if (window->row_hover == vehicle_type)
                        {
                            gfx::fill_rect(dpi, 0, y, window->width, y + window->row_height - 1, 0x2000030);
                            coloured_string = string_ids::wcolour2_stringid2;
                        }

                        int16_t half = (window->row_height - 22) / 2;
                        auto x = draw_vehicle_inline(dpi, vehicle_type, 0, companymgr::get_controlling_id(), { 0, static_cast<int16_t>(y + half) });

                        auto vehicle_obj = objectmgr::get<vehicle_object>(vehicle_type);
                        _common_format_args[0] = vehicle_obj->name;
                        half = (window->row_height - 10) / 2;
                        gfx::draw_string_494B3F(*dpi, x + 3, y + half, colour::black, coloured_string, _common_format_args);
                    }
                }
                break;
            }
            case scrollIdx::vehicle_preview:
            {
                auto colour = colour::get_shade(window->colours[1], 0);
                // gfx::clear needs the colour copied to each byte of eax
                gfx::clear(*dpi, colour * 0x01010101);

                if (window->row_hover == -1)
                {
                    break;
                }

                uint8_t unk1 = _52622E & 0x3F;
                uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;
                draw_vehicle_overview(dpi, window->row_hover, companymgr::get_controlling_id(), unk1, unk2, { 90, 37 });

                auto vehicle_obj = objectmgr::get<vehicle_object>(window->row_hover);
                auto buffer = const_cast<char*>(stringmgr::get_string(string_ids::buffer_1250));
                buffer = stringmgr::format_string(buffer, vehicle_obj->name);
                auto usable_cargo_types = vehicle_obj->primary_cargo_types | vehicle_obj->secondary_cargo_types;

                for (auto cargo_type = utility::bitscanforward(usable_cargo_types); cargo_type != -1; cargo_type = utility::bitscanforward(usable_cargo_types))
                {
                    usable_cargo_types &= ~(1 << cargo_type);
                    auto cargo_obj = objectmgr::get<cargo_object>(cargo_type);
                    *buffer++ = ' ';
                    *buffer++ = control_codes::inline_sprite_str;
                    *(reinterpret_cast<uint32_t*>(buffer)) = cargo_obj->unit_inline_sprite;
                    buffer += 4;
                }

                *buffer++ = '\0';
                _common_format_args[0] = string_ids::buffer_1250;
                gfx::draw_string_centred_clipped(*dpi, 89, 52, 177, 0x20, string_ids::wcolour2_stringid2, _common_format_args);
                break;
            }
        }
    }

    // 0x4C28D2
    static void SetDisabledTransportTabs(ui::window* window)
    {
        auto available_vehicles = companymgr::companies()[window->number].available_vehicles;
        // By shifting by 4 the available_vehicles flags align with the tabs flags
        auto disabled_tabs = (available_vehicles << 4) ^ ((1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships));
        window->disabled_widgets = disabled_tabs;
    }

    // 0x4C2D8A
    static void SetTrackTypeTabs(ui::window* window)
    {
        VehicleType current_tab_type = static_cast<VehicleType>(window->current_tab);
        GenerateBuildableVehiclesArray(current_tab_type, 0xFF, nullptr);

        auto rail_track_types = 0;
        auto road_track_types = 0;
        for (auto veh_obj_index = 0; veh_obj_index < _num_available_vehicles; veh_obj_index++)
        {
            auto vehicle_obj = objectmgr::get<vehicle_object>(_available_vehicles[veh_obj_index]);
            if (vehicle_obj && vehicle_obj->mode == TransportMode::rail)
            {
                rail_track_types |= (1 << vehicle_obj->track_type);
            }
            else if (vehicle_obj && vehicle_obj->mode == TransportMode::road)
            {
                auto track_type = vehicle_obj->track_type;
                if (track_type == 0xFF)
                {
                    track_type = _525FC5;
                }
                road_track_types |= (1 << track_type);
            }
            else
            {
                // Reset the tabs
                _tab_track_types[0] = -1;
                window->widgets[tab_vehicles_for_0].type = widget_type::wt_8;
                for (widget_index i = tab_vehicles_for_1; i <= tab_vehicles_for_7; ++i)
                {
                    window->widgets[i].type = widget_type::none;
                }
                return;
            }
        }

        widget_index tab_widget_index = tab_vehicles_for_0;
        auto track_type = 0;
        for (track_type = utility::bitscanforward(rail_track_types); track_type != -1 && tab_widget_index <= tab_vehicles_for_7; track_type = utility::bitscanforward(rail_track_types))
        {
            rail_track_types &= ~(1 << track_type);
            window->widgets[tab_widget_index].type = widget_type::wt_8;
            _tab_track_types[widget_index_to_tab_vehicle_for(tab_widget_index)] = track_type;
            tab_widget_index++;
        }

        if (track_type == -1 && tab_widget_index <= tab_vehicles_for_7)
        {
            for (track_type = utility::bitscanforward(road_track_types); track_type != -1 && tab_widget_index <= tab_vehicles_for_7; track_type = utility::bitscanforward(road_track_types))
            {
                road_track_types &= ~(1 << track_type);
                window->widgets[tab_widget_index].type = widget_type::wt_8;
                _tab_track_types[widget_index_to_tab_vehicle_for(tab_widget_index)] = track_type | (1 << 7);
                tab_widget_index++;
            }
        }

        _num_track_type_tabs = widget_index_to_tab_vehicle_for(tab_widget_index);

        for (; tab_widget_index <= tab_vehicles_for_7; ++tab_widget_index)
        {
            window->widgets[tab_widget_index].type = widget_type::none;
        }
    }

    // 0x4C1CBE
    // if previous track tab on previous transport type tab is also compatible keeps it on that track type
    static void ResetTrackTypeTabSelection(ui::window* window)
    {
        if (window->current_tab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains) || window->current_tab == (widx::tab_build_new_ships - widx::tab_build_new_trains))
        {
            window->current_secondary_tab = 0;
        }

        bool found = false;
        uint32_t track_tab = 0;
        for (; track_tab < _num_track_type_tabs; track_tab++)
        {
            if (last_railroad_option == _tab_track_types[track_tab])
            {
                found = true;
                break;
            }

            if (last_road_option == _tab_track_types[track_tab])
            {
                found = true;
                break;
            }
        }

        window->current_secondary_tab = found ? track_tab : 0;

        bool is_road = _tab_track_types[track_tab] & (1 << 7);
        uint8_t track_type = _tab_track_types[track_tab] & ~(1 << 7);
        SetTopToolbarLastTrack(track_type, is_road);
    }

    // 0x4A3A06
    static void SetTopToolbarLastTrack(uint8_t track_type, bool is_road)
    {
        bool set_rail = false;
        if (is_road)
        {
            auto road_obj = objectmgr::get<road_object>(track_type);
            if (road_obj && road_obj->flags & flags_12::unk_01)
            {
                set_rail = true;
            }
        }
        else
        {
            auto rail_obj = objectmgr::get<track_object>(track_type);
            if (rail_obj && !(rail_obj->flags & flags_22::unk_02))
            {
                set_rail = true;
            }
        }

        if (set_rail)
        {
            last_railroad_option = track_type;
        }
        else
        {
            last_road_option = track_type;
        }

        // The window number doesn't really matter as there is only one top toolbar
        WindowManager::invalidate(WindowType::topToolbar, 0);
    }

    // 0x4C2865 common for build vehicle window and vehicle list
    static void SetTransportTypeTabs(ui::window* window)
    {
        auto disabled_widgets = window->disabled_widgets >> widx::tab_build_new_trains;
        auto widget = window->widgets + widx::tab_build_new_trains;
        auto tab_width = widget->right - widget->left;
        auto tab_x = widget->left;
        for (auto i = 0; i <= widx::tab_build_new_ships - widx::tab_build_new_trains; ++i, ++widget)
        {
            if (disabled_widgets & (1ULL << i))
            {
                widget->type = widget_type::none;
            }
            else
            {
                widget->type = widget_type::wt_8;
                widget->left = tab_x;
                widget->right = tab_x + tab_width;
                tab_x += tab_width + 1;
            }
        }
    }

    /* 0x4B60CC
     * Opens vehicle window and clicks???
     */
    static void sub_4B60CC(openloco::vehicle* vehicle)
    {
        registers regs;
        regs.edx = (int32_t)vehicle;
        call(0x4B60CC, regs);
    }

    // 0x4C2BFD
    static void draw_build_tabs(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();
        auto companyColour = companymgr::get_company_colour(window->number);

        for (auto tab : typeTabInformationByType)
        {
            auto frame_no = 0;
            if (window->current_tab == tab.widgetIndex - widx::tab_build_new_trains)
            {
                frame_no = (window->frame_no / 2) & 0xF;
            }
            uint32_t image = gfx::recolour(skin->img + tab.imageId + frame_no, companyColour);
            widget::draw_tab(window, dpi, image, tab.widgetIndex);
        }
    }

    // 0x4C28F1
    static void draw_build_sub_type_tabs(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();
        auto companyColour = companymgr::get_company_colour(window->number);

        auto left = window->x;
        auto top = window->y + 69;
        auto right = left + window->width - 187;
        auto bottom = top;
        gfx::fill_rect(dpi, left, top, right, bottom, colour::get_shade(window->colours[1], 7));

        left = window->x + window->width - 187;
        top = window->y + 41;
        right = left;
        bottom = top + 27;
        gfx::fill_rect(dpi, left, top, right, bottom, colour::get_shade(window->colours[1], 7));

        for (uint32_t tab = 0; tab < _num_track_type_tabs; ++tab)
        {
            const auto widget = window->widgets[tab + widx::tab_vehicles_for_0];
            if (window->current_secondary_tab == tab)
            {
                left = widget.left + window->x + 1;
                top = widget.top + window->y + 26;
                right = left + 29;
                bottom = top;
                gfx::fill_rect(dpi, left, top, right, bottom, colour::get_shade(window->colours[1], 5));
            }

            auto img = 0;
            auto type = _tab_track_types[tab];
            if (type == -1)
            {
                if (window->current_tab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains))
                {
                    img = skin->img + interface_skin::image_ids::toolbar_menu_airport;
                }
                else
                {
                    img = skin->img + interface_skin::image_ids::toolbar_menu_ship_port;
                }
                // Original saved the company colour in the img but didn't set the recolour flag
            }
            else if (type & (1 << 7)) // is_road
            {
                type &= ~(1 << 7);
                auto road_obj = objectmgr::get<road_object>(type);
                img = road_obj->var_0E;
                if (window->current_secondary_tab == tab)
                {
                    img += (window->frame_no / 4) & 0x1F;
                }
                img = gfx::recolour(img, companyColour);
            }
            else
            {
                auto track_obj = objectmgr::get<track_object>(type);
                img = track_obj->var_1E;
                if (window->current_secondary_tab == tab)
                {
                    img += (window->frame_no / 4) & 0xF;
                }
                img = gfx::recolour(img, companyColour);
            }

            widget::draw_tab(window, dpi, img, tab + widx::tab_vehicles_for_0);
        }
    }

    // 0x4B7741
    static void draw_vehicle_overview(gfx::drawpixelinfo_t* dpi, int16_t vehicle_type_idx, company_id_t company, uint8_t eax, uint8_t esi, gfx::point_t offset)
    {
        registers regs;
        regs.cx = offset.x;
        regs.dx = offset.y;
        regs.eax = eax;
        regs.esi = esi;
        regs.ebx = company;
        regs.ebp = vehicle_type_idx;
        regs.edi = (uintptr_t)dpi;
        call(0x4B7741, regs);
    }

    // 0x4B7711
    static int16_t draw_vehicle_inline(gfx::drawpixelinfo_t* dpi, int16_t vehicle_type_idx, uint8_t unk_1, company_id_t company, gfx::point_t loc)
    {
        registers regs;

        regs.al = unk_1;
        regs.ebx = company;
        regs.cx = loc.x;
        regs.dx = loc.y;
        regs.edi = (uintptr_t)dpi;
        regs.ebp = vehicle_type_idx;
        call(0x4B7711, regs);
        // Returns right coordinate of the drawing
        return regs.cx;
    }
}
