#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/land_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../objects/water_object.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top
{
    namespace widx
    {
        enum
        {
            loadsave_menu,
            audio_menu,
            zoom_menu,
            rotate_menu,
            view_menu,

            terraform_menu,
            railroad_menu,
            road_menu,
            port_menu,
            build_vehicles_menu,

            vehicles_menu,
            stations_menu,
            towns_menu,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 30, 28 }, widget_type::wt_7, 0),
        make_widget({ 30, 0 }, { 30, 28 }, widget_type::wt_7, 0),
        make_widget({ 74, 0 }, { 30, 28 }, widget_type::wt_7, 1),
        make_widget({ 104, 0 }, { 30, 28 }, widget_type::wt_7, 1),
        make_widget({ 134, 0 }, { 30, 28 }, widget_type::wt_7, 1),

        make_widget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 387, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 357, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 417, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 417, 0 }, { 30, 28 }, widget_type::wt_7, 2),

        make_widget({ 490, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        make_widget({ 520, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        make_widget({ 460, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        widget_end(),
    };

    static window_event_list _events;

    static void on_resize(window* window);
    static void on_mouse_down(window* window, widget_index widgetIndex);
    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    static void on_update(window* window);
    static void prepare_draw(window* window);
    static void draw(window* window, gfx::drawpixelinfo_t* dpi);

    // 0x00438B26
    void open()
    {
        _events.on_resize = on_resize;
        _events.event_03 = on_mouse_down;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = on_update;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            gfx::ui_size_t(ui::width(), 28),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::loadsave_menu) | (1 << widx::audio_menu) | (1 << widx::zoom_menu) | (1 << widx::rotate_menu) | (1 << widx::view_menu) | (1 << widx::terraform_menu) | (1 << widx::railroad_menu) | (1 << widx::road_menu) | (1 << widx::port_menu) | (1 << widx::build_vehicles_menu) | (1 << widx::vehicles_menu) | (1 << widx::stations_menu) | (1 << widx::towns_menu);
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = skin->colour_12;
            window->colours[1] = skin->colour_13;
            window->colours[2] = skin->colour_14;
            window->colours[3] = skin->colour_15;
        }
    }

    // 0x0043A17E
    static void on_resize(window* window)
    {
        auto main = WindowManager::getMainWindow();
        if (main == nullptr)
            window->set_disabled_widgets_and_invalidate(1 << 2 | 1 << 3);
        else
            window->set_disabled_widgets_and_invalidate(0);
    }

    // 0x0043B0F7
    static void loadsave_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::menu_load_game);
        dropdown::add(1, string_ids::menu_save_game);
        dropdown::add(2, 0);
        dropdown::add(3, string_ids::menu_about);
        dropdown::add(4, string_ids::options);
        dropdown::add(5, string_ids::menu_screenshot);
        dropdown::add(6, 0);
        dropdown::add(7, string_ids::menu_quit_game);
        dropdown::show_below(window, widgetIndex, 8);
        dropdown::set_highlighted_item(1);
    }

    // 0x0043B04B
    static void audio_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::dropdown_without_checkmark, string_ids::menu_mute);
        dropdown::add(1, string_ids::dropdown_without_checkmark, string_ids::menu_play_music);
        dropdown::add(2, 0);
        dropdown::add(3, string_ids::menu_music_options);
        dropdown::show_below(window, widgetIndex, 4);

        if (addr<0x50D555, bool>())
            dropdown::set_selection(0);

        if (addr<0x50AED7, bool>())
            dropdown::set_selection(1);

        dropdown::set_highlighted_item(0);
    }

    // 0x0043A78E
    static void zoom_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_zoom_in, string_ids::menu_zoom_in });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_zoom_out, string_ids::menu_zoom_out });

        static uint32_t map_sprites_by_rotation[] = {
            interface_skin::image_ids::toolbar_menu_map_north,
            interface_skin::image_ids::toolbar_menu_map_west,
            interface_skin::image_ids::toolbar_menu_map_south,
            interface_skin::image_ids::toolbar_menu_map_east,
        };

        loco_global<int32_t, 0x00e3f0b8> current_rotation;
        uint32_t map_sprite = map_sprites_by_rotation[current_rotation];

        dropdown::add(2, string_ids::menu_sprite_stringid, { interface->img + map_sprite, string_ids::menu_map });
        dropdown::show_below(window, widgetIndex, 3, 25);
        dropdown::set_highlighted_item(0);

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow->viewports[0]->zoom == 0)
        {
            dropdown::set_disabled_item(0);
            dropdown::set_highlighted_item(1);
        }

        if (mainWindow->viewports[0]->zoom == 3)
            dropdown::set_disabled_item(1);
    }

    // 0x0043A5C5
    static void rotate_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_rotate_clockwise, string_ids::menu_rotate_clockwise });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_rotate_anti_clockwise, string_ids::menu_rotate_anti_clockwise });
        dropdown::show_below(window, widgetIndex, 2, 25);
        dropdown::set_highlighted_item(0);
    }

    // 0x0043ADF6
    static void view_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::dropdown_without_checkmark, string_ids::menu_underground_view);
        dropdown::add(1, string_ids::dropdown_without_checkmark, string_ids::menu_hide_foreground_tracks_roads);
        dropdown::add(2, string_ids::dropdown_without_checkmark, string_ids::menu_hide_foreground_scenery_buildings);
        dropdown::add(3, 0);
        dropdown::add(4, string_ids::dropdown_without_checkmark, string_ids::menu_height_marks_on_land);
        dropdown::add(5, string_ids::dropdown_without_checkmark, string_ids::menu_height_marks_on_tracks_roads);
        dropdown::add(6, string_ids::dropdown_without_checkmark, string_ids::menu_one_way_direction_arrows);
        dropdown::add(7, 0);
        dropdown::add(8, string_ids::dropdown_without_checkmark, string_ids::menu_town_names_displayed);
        dropdown::add(9, string_ids::dropdown_without_checkmark, string_ids::menu_station_names_displayed);
        dropdown::show_below(window, widgetIndex, 10);

        uint32_t current_viewport_flags = WindowManager::getMainWindow()->viewports[0]->flags;

        if (current_viewport_flags & viewport_flags::underground_view)
            dropdown::set_selection(0);

        if (current_viewport_flags & viewport_flags::hide_foreground_tracks_roads)
            dropdown::set_selection(1);

        if (current_viewport_flags & viewport_flags::hide_foreground_scenery_buildings)
            dropdown::set_selection(2);

        if (current_viewport_flags & viewport_flags::height_marks_on_tracks_roads)
            dropdown::set_selection(4);

        if (current_viewport_flags & viewport_flags::height_marks_on_land)
            dropdown::set_selection(5);

        if (current_viewport_flags & viewport_flags::one_way_direction_arrows)
            dropdown::set_selection(6);

        if (!(current_viewport_flags & viewport_flags::town_names_displayed))
            dropdown::set_selection(8);

        if (!(current_viewport_flags & viewport_flags::station_names_displayed))
            dropdown::set_selection(9);

        dropdown::set_highlighted_item(0);
    }

    // 0x0043A3C3
    static void terraform_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        auto land = objectmgr::get<land_object>(addr<0x00525FB6, uint8_t>());
        auto water = objectmgr::get<water_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_bulldozer, string_ids::menu_clear_area });
        dropdown::add(1, string_ids::menu_sprite_stringid, { land->var_16 + land::image_ids::toolbar_terraform_land, string_ids::menu_adjust_land });
        dropdown::add(2, string_ids::menu_sprite_stringid, { water->var_06 + water::image_ids::toolbar_terraform_water, string_ids::menu_adjust_water });
        dropdown::add(3, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_plant_trees, string_ids::menu_plant_trees });
        dropdown::add(4, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_build_walls, string_ids::menu_build_walls });
        dropdown::show_below(window, widgetIndex, 5, 25);
        dropdown::set_highlighted_item(0);
    }

    // 0x0043A4E9
    static void stations_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        uint32_t sprite_base = interface->img;

        // Apply company colours.
        loco_global<uint8_t[16], 0x009C645C> company_colours;
        uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
        sprite_base |= (1 << 29) | (colour << 19);

        dropdown::add(0, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::all_stations, string_ids::all_stations });
        dropdown::add(1, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::rail_stations, string_ids::rail_stations });
        dropdown::add(2, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::road_stations, string_ids::road_stations });
        dropdown::add(3, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::airports, string_ids::airports });
        dropdown::add(4, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::ship_ports, string_ids::ship_ports });
        dropdown::show_below(window, widgetIndex, 5, 25);
        dropdown::set_highlighted_item(0);
    }

    // 0x0043A8CE
    static void towns_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_towns, string_ids::menu_towns });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_industries, string_ids::menu_industries });
        dropdown::show_below(window, widgetIndex, 2, 25);
        dropdown::set_highlighted_item(0);
    }

    // 0x0043A071
    static void on_mouse_down(window* window, widget_index widgetIndex)
    {
        registers regs;
        regs.esi = (int32_t)window;
        regs.edx = widgetIndex;
        regs.edi = (int32_t)&window->widgets[widgetIndex];

        switch (widgetIndex)
        {
            case widx::loadsave_menu:
                loadsave_menu_mouse_down(window, widgetIndex);
                break;

            case widx::audio_menu:
                audio_menu_mouse_down(window, widgetIndex);
                break;

            case widx::zoom_menu:
                zoom_menu_mouse_down(window, widgetIndex);
                break;

            case widx::rotate_menu:
                rotate_menu_mouse_down(window, widgetIndex);
                break;

            case widx::view_menu:
                view_menu_mouse_down(window, widgetIndex);
                break;

            case widx::terraform_menu:
                terraform_menu_mouse_down(window, widgetIndex);
                break;

            case widx::railroad_menu:
                call(0x43A2B0, regs);
                break;

            case widx::road_menu:
                call(0x43A19F, regs);
                break;

            case widx::port_menu:
                call(0x43A965, regs);
                break;

            case widx::build_vehicles_menu:
                call(0x43AD1F, regs);
                break;

            case widx::vehicles_menu:
                call(0x43ABCB, regs);
                break;

            case widx::stations_menu:
                stations_menu_mouse_down(window, widgetIndex);
                break;

            case widx::towns_menu:
                towns_menu_mouse_down(window, widgetIndex);
                break;
        }
    }

    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        registers regs;
        regs.esi = (int32_t)window;
        regs.edx = widgetIndex;
        regs.ax = itemIndex;

        switch (widgetIndex)
        {
            case widx::loadsave_menu:
                call(0x43B154, regs);
                break;

            case widx::audio_menu:
                call(0x43B0B8, regs);
                break;

            case widx::zoom_menu:
                call(0x43A86D, regs);
                break;

            case widx::rotate_menu:
                call(0x43A624, regs);
                break;

            case widx::view_menu:
                call(0x43AF37, regs);
                break;

            case widx::terraform_menu:
                call(0x43A4A8, regs);
                break;

            case widx::railroad_menu:
                call(0x43A39F, regs);
                break;

            case widx::road_menu:
                call(0x43A28C, regs);
                break;

            case widx::port_menu:
                call(0x43AA0A, regs);
                break;

            case widx::build_vehicles_menu:
                call(0x43ADC7, regs);
                break;

            case widx::vehicles_menu:
                call(0x43ACEF, regs);
                break;

            case widx::stations_menu:
                call(0x43A596, regs);
                break;

            case widx::towns_menu:
                call(0x43A932, regs);
                break;
        }
    }

    static void on_update(window* window)
    {
        loco_global<int32_t, 0x9C86F8> _9C86F8;
        _9C86F8++;
    }

    // 0x00439DE4
    static void draw(window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        if (_widgets[widx::road_menu].type != widget_type::none)
        {
            uint32_t x = _widgets[widx::road_menu].left + window->x;
            uint32_t y = _widgets[widx::road_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = addr<0x00525FAB, uint8_t>();
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = objectmgr::get<road_object>(ebx);
                fg_image = (1 << 29) | obj->var_0E;
            }
            else
            {
                auto obj = objectmgr::get<track_object>(ebx);
                fg_image = (1 << 29) | obj->var_1E;
            }

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            y--;
            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[2] << 19;

            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::road_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::road_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        if (_widgets[widx::railroad_menu].type != widget_type::none)
        {
            uint32_t x = _widgets[widx::railroad_menu].left + window->x;
            uint32_t y = _widgets[widx::railroad_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = addr<0x00525FAA, uint8_t>();
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = objectmgr::get<road_object>(ebx);
                fg_image = (1 << 29) | obj->var_0E;
            }
            else
            {
                auto obj = objectmgr::get<track_object>(ebx);
                fg_image = (1 << 29) | obj->var_1E;
            }

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[2] << 19;

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::railroad_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        {
            uint32_t x = _widgets[widx::vehicles_menu].left + window->x;
            uint32_t y = _widgets[widx::vehicles_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = addr<0x00525FAF, uint8_t>();
            switch (ebx)
            {
                case 0:
                    fg_image = 0x1A2;
                    break;
                case 1:
                    fg_image = 0x1B2;
                    break;
                case 2:
                    fg_image = 0x1C2;
                    break;
                case 3:
                    fg_image = 0x1BA;
                    break;
                case 4:
                    fg_image = 0x1AA;
                    break;
                case 5:
                    fg_image = 0x1CA;
            }

            auto interface = objectmgr::get<interface_skin_object>();
            fg_image += (1 << 29) | interface->img;

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[3] << 19;

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::vehicles_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        {
            uint32_t x = _widgets[widx::build_vehicles_menu].left + window->x;
            uint32_t y = _widgets[widx::build_vehicles_menu].top + window->y;

            // Figure out what icon to show on the button face.
            uint32_t fg_image = addr<0x0052622C, uint8_t>();
            fg_image <<= 1;
            auto interface = objectmgr::get<interface_skin_object>();
            fg_image += (1 << 29) | (interface->img + 0x1F);

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[2] << 19;

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::build_vehicles_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::build_vehicles_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }
    }

    // 0x00439BCB
    static void prepare_draw(window* window)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        uint32_t colour = window->colours[0] << 19;

        loco_global<bool, 0x0050D555> enable_sound;
        if (enable_sound)
        {
            window->activated_widgets |= (1 << widx::audio_menu);
            _widgets[widx::audio_menu].image = (1 << 29) | colour | (interface->img + interface_skin::image_ids::toolbar_audio_inactive);
        }
        else
        {
            window->activated_widgets &= ~(1 << widx::audio_menu);
            _widgets[widx::audio_menu].image = (1 << 29) | colour | (interface->img + interface_skin::image_ids::toolbar_audio_active);
        }

        loco_global<bool, 0x009C870D> _9C870D;
        if (!_9C870D && addr<0x00525FAC, int8_t>() != -1 && addr<0x00525FAD, int8_t>() == -1)
            _9C870D = true;

        _widgets[widx::loadsave_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_loadsave);
        _widgets[widx::zoom_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_zoom);
        _widgets[widx::rotate_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_rotate);
        _widgets[widx::view_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_view);

        _widgets[widx::terraform_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_terraform);
        _widgets[widx::railroad_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::road_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::port_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::build_vehicles_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);

        _widgets[widx::vehicles_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::stations_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_stations);

        if (addr<0x009C870C, int8_t>() == 0)
            _widgets[widx::towns_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_towns);
        else
            _widgets[widx::towns_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_industries);

        if (!_9C870D)
            _widgets[widx::port_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_airports);
        else
            _widgets[widx::port_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_ports);

        if (addr<0x00525FAB, int8_t>() != -1)
            _widgets[widx::road_menu].type = widget_type::wt_7;
        else
            _widgets[widx::road_menu].type = widget_type::none;

        if (addr<0x00525FAA, int8_t>() != -1)
            _widgets[widx::railroad_menu].type = widget_type::wt_7;
        else
            _widgets[widx::railroad_menu].type = widget_type::none;

        if (addr<0x00525FAC, int8_t>() != -1 || addr<0x00525FAD, int8_t>() != -1)
            _widgets[widx::port_menu].type = widget_type::wt_7;
        else
            _widgets[widx::port_menu].type = widget_type::none;

        uint32_t x = std::max(640, ui::width()) - 1;
        _widgets[widx::towns_menu].right = x;
        x -= 29;
        _widgets[widx::towns_menu].left = x;
        x -= 1;

        _widgets[widx::stations_menu].right = x;
        x -= 29;
        _widgets[widx::stations_menu].left = x;
        x -= 1;

        _widgets[widx::vehicles_menu].right = x;
        x -= 29;
        _widgets[widx::vehicles_menu].left = x;
        x -= 11;

        _widgets[widx::build_vehicles_menu].right = x;
        x -= 29;
        _widgets[widx::build_vehicles_menu].left = x;
        x -= 1;

        if (_widgets[widx::port_menu].type != widget_type::none)
        {
            _widgets[widx::port_menu].right = x;
            x -= 29;
            _widgets[widx::port_menu].left = x;
            x -= 1;
        }

        if (_widgets[widx::road_menu].type != widget_type::none)
        {
            _widgets[widx::road_menu].right = x;
            x -= 29;
            _widgets[widx::road_menu].left = x;
            x -= 1;
        }

        if (_widgets[widx::railroad_menu].type != widget_type::none)
        {
            _widgets[widx::railroad_menu].right = x;
            x -= 29;
            _widgets[widx::railroad_menu].left = x;
            x -= 1;
        }

        _widgets[widx::terraform_menu].right = x;
        x -= 29;
        _widgets[widx::terraform_menu].left = x;
        x -= 1;
    }
}
