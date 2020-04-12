#include "../companymgr.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"

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
        scrollview_1,
        scrollview_2
    };

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
        make_widget({ 3, 72 }, { 374, 146 }, widget_type::scrollview, 1, vertical),
        make_widget({ 250, 44 }, { 180, 66 }, widget_type::scrollview, 1),
    };

    static loco_global<int16_t, 0x01136268>
        _1136268;
    static loco_global<uint16_t[1], 0x0113626A> _113626A;
    static loco_global<int32_t, 0x011364E8> _11364E8;
    static loco_global<uint32_t, 0x011364EC> _11364EC;
    static loco_global<int8_t[1], 0x011364F0> _11364F0;
    static loco_global<uint8_t, 0x00525FC5> _525FC5;
    static std::array<uint16_t, 6> _504458{ 0x16, 0x16, 0x16, 0x16, 0x2A, 0x1E };

    static window_event_list _events;

    static void sub_4B92A5(ui::window* window);
    static void sub_4C28D2(ui::window* window);
    static void sub_4C2D8A(ui::window* window);
    static void sub_4C1CBE(ui::window* window);

    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_resize(window* window);
    static void on_periodic_update(window* window);
    static void on_update(window* window);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void tooltip(ui::window* window, widget_index widgetIndex);
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
        window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_vehicles_for_0) | (1 << widx::tab_vehicles_for_1) | (1 << widx::tab_vehicles_for_2) | (1 << widx::tab_vehicles_for_3) | (1 << widx::tab_vehicles_for_4) | (1 << widx::tab_vehicles_for_5) | (1 << widx::tab_vehicles_for_6) | (1 << widx::tab_vehicles_for_7) | (1 << widx::scrollview_1);
        window->owner = companymgr::get_controlling_id();
        window->frame_no = 0;
        auto skin = openloco::objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[1] = skin->colour_0A;
        }
        sub_4C28D2(window);
        return window;
    }

    /* 0x4C1AF7
     * depending on flags (1<<31) vehicle is a tab id or a vehicle thing_id
     */
    window* open(uint32_t vehicle, uint32_t flags)
    {
        auto window = WindowManager::bringToFront(WindowType::buildVehicle, companymgr::get_controlling_id());

        if (window)
        {
            widget_index tab = widx::tab_build_new_trains;
            if (!(flags & (1ULL << 31)))
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

            if (flags & (1 << 31))
            {
                _11364E8 = -1;
            }
            else
            {
                _11364E8 = vehicle;
            }
        }
        else
        {
            window = create(companymgr::get_controlling_id());
            window->width = window_size.width;
            window->height = window_size.height;
            _11364E8 = -1;
            if (!(flags & (1ULL << 31)))
            {
                _11364E8 = vehicle;
                auto veh = thingmgr::get<openloco::vehicle>(vehicle);
                window->current_tab = veh->var_5E;
            }
            else
            {
                window->current_tab = vehicle;
            }

            window->var_83E = _504458[window->current_tab];
            window->row_count = 0;
            window->var_83C = 0;
            window->row_hover = -1;
            window->invalidate();
            window->widgets = _widgets;
            window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_vehicles_for_0) | (1 << widx::tab_vehicles_for_1) | (1 << widx::tab_vehicles_for_2) | (1 << widx::tab_vehicles_for_3) | (1 << widx::tab_vehicles_for_4) | (1 << widx::tab_vehicles_for_5) | (1 << widx::tab_vehicles_for_6) | (1 << widx::tab_vehicles_for_7) | (1 << widx::scrollview_1);
            window->holdable_widgets = 0;
            window->event_handlers = &_events;
            window->activated_widgets = 0;
            sub_4C28D2(window);
            sub_4C2D8A(window);
            sub_4C1CBE(window);
            sub_4B92A5(window);

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
        }

        if (_11364E8 == -1)
        {
            return window;
        }
        auto veh = thingmgr::get<openloco::vehicle>(_11364E8);
        if (veh == nullptr)
        {
            return window;
        }
        auto unk_al = veh->var_35;
        if (veh->mode != TransportMode::rail)
        {
            unk_al |= (1 << 7);
            if (unk_al == 0xFF)
            {
                unk_al = _525FC5;
            }
        }

        bool found = false;
        uint32_t unk_tab_offset = 0;
        for (; unk_tab_offset < _11364EC;)
        {
            unk_tab_offset++;
            if (unk_al == _11364F0[unk_tab_offset - 1])
            {
                unk_tab_offset--;
                found = true;
                break;
            }
        }
        widget_index widgetIndex = widx::tab_vehicles_for_0;
        if (found)
        {
            widgetIndex = widx::tab_vehicles_for_0 + unk_tab_offset;
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

    static void sub_4B9165(uint8_t dl, uint8_t dh, void* esi)
    {
        registers regs;
        regs.dl = dl;
        regs.dh = dh;
        regs.esi = (uintptr_t)esi;
        if (esi == nullptr)
        {
            regs.esi = -1;
        }

        call(0x004B9165, regs);
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

        if (_11364E8 != vehicleId)
        {
            _11364E8 = vehicleId;
            window->var_83C = 0;
            window->invalidate();
        }

        uint16_t vehicleType = window->current_tab;
        uint8_t dh = _11364F0[window->var_874];

        thing_base* vehicle = nullptr;
        if (_11364E8 != -1)
        {
            vehicle = thingmgr::get<thing_base>(_11364E8);
        }

        sub_4B9165(vehicleType, dh, vehicle);

        int cx = _1136268;
        if (window->var_83C == cx)
            return;

        uint16_t* src = _113626A;
        int16_t* dest = window->row_info;
        window->var_83C = cx;
        window->row_count = 0;
        while (cx != 0)
        {
            *dest = *src;
            dest++;
            src++;
            cx--;
        }
        window->row_hover = -1;
        window->invalidate();
    }

    // 0x4C3576
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (uint32_t)window;

        call(0x004C3576, regs);
    }

    // 0x4C3929
    static void on_resize(window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x4C3929, regs);
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
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, (window->var_874 & 0xFF) + 10);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, 19);
    }

    // 0x4C37B9
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->var_83C * window->var_83E;
    }

    // 0x4C384B
    static void on_scroll_mouse_down(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)window;
        regs.cx = x;
        regs.dx = y;
        call(0x4C384B, regs);
    }

    // 0x4C3802
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)window;
        regs.cx = x;
        regs.dx = y;
        call(0x4C3802, regs);
    }

    // 0x4C370C
    static void tooltip(ui::window* window, widget_index widgetIndex)
    {
        registers regs;
        regs.ax = widgetIndex;
        regs.esi = (int32_t)window;
        call(0x4C370C, regs);
    }

    // 0x4C37CB
    static ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        registers regs;
        regs.cx = xPos;
        regs.dx = yPos;
        regs.ax = widgetIdx;
        regs.ebx = -1;
        regs.edi = (int32_t)&window->widgets[widgetIdx];
        regs.esi = (int32_t)window;
        call(0x4C37CB, regs);

        if (regs.ebx == -1)
        {
            return fallback;
        }

        return (cursor_id)regs.ebx;
    }

    // 0x4C2E5C
    static void prepare_draw(ui::window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x4C2E5C, regs);
    }

    // 0x4C2F23
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        registers regs;
        regs.esi = (int32_t)window;
        regs.edi = (int32_t)dpi;
        call(0x4C2F23, regs);
    }

    // 0x4C3307
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        registers regs;
        regs.ax = scrollIndex;
        regs.esi = (int32_t)window;
        regs.edi = (int32_t)dpi;
        call(0x4C3307, regs);
    }

    // 0x4C28D2
    static void sub_4C28D2(ui::window* window)
    {
        window->number;
        auto available_vehicles = companymgr::companies()[window->number].available_vehicles;
        // By shifting by 4 the available_vehicles flags align with the tabs flags
        auto disabled_tabs = (available_vehicles << 4) ^ ((1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships));
        window->disabled_widgets = disabled_tabs;
    }

    static void sub_4C2D8A(ui::window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x4C2D8A, regs);
    }

    static void sub_4C1CBE(ui::window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x4C1CBE, regs);
    }
}
