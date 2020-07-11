#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../game_commands.h"
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
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "toolbar_top_common.h"
#include <map>

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top::game
{
    static loco_global<uint8_t[40], 0x00113DB20> menu_options;

    static loco_global<uint8_t, 0x00525FAA> last_railroad_option;
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x00525FAF> last_vehicles_option;
    static loco_global<uint8_t, 0x0052622C> last_build_vehicles_option;

    static loco_global<uint32_t, 0x009C86F8> zoom_ticks;

    static loco_global<uint8_t, 0x009C870C> last_town_option;
    static loco_global<uint8_t, 0x009C870D> last_port_option;

    static loco_global<int8_t[18], 0x0050A006> available_objects;

    namespace widx
    {
        enum
        {
            railroad_menu = common::widx::w6
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

    static void on_mouse_down(window* window, widget_index widgetIndex);
    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    static void prepare_draw(window* window);
    static void draw(window* window, gfx::drawpixelinfo_t* dpi);

    // 0x00438B26
    void open()
    {
        zoom_ticks = 0;
        last_town_option = 0;
        last_port_option = 0;

        _events.on_resize = common::on_resize;
        _events.event_03 = on_mouse_down;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = common::on_update;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            gfx::ui_size_t(ui::width(), 28),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << common::widx::loadsave_menu) | (1 << common::widx::audio_menu) | (1 << common::widx::zoom_menu) | (1 << common::widx::rotate_menu) | (1 << common::widx::view_menu) | (1 << common::widx::terraform_menu) | (1 << widx::railroad_menu) | (1 << common::widx::road_menu) | (1 << common::widx::port_menu) | (1 << common::widx::build_vehicles_menu) | (1 << common::widx::vehicles_menu) | (1 << common::widx::stations_menu) | (1 << common::widx::towns_menu);
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
        dropdown::add(7, string_ids::menu_quit_to_menu);
        dropdown::add(8, string_ids::menu_exit_openloco);
        dropdown::show_below(window, widgetIndex, 9, 0);
        dropdown::set_highlighted_item(1);
    }

    // 0x0043B154
    static void loadsave_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                // Load game
                game_commands::do_21(0, 0);
                break;

            case 1:
                // Save game
                call(0x0043B1C4);
                break;

            case 3:
                about::open();
                break;

            case 4:
                options::open();
                break;

            case 5:
            {
                loco_global<uint8_t, 0x00508F16> screenshot_countdown;
                screenshot_countdown = 10;
                break;
            }

            case 7:
                // Return to title screen
                game_commands::do_21(0, 1);
                break;

            case 8:
                // Exit to desktop
                game_commands::do_21(0, 2);
                break;
        }
    }

    // 0x0043B04B
    static void audio_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::dropdown_without_checkmark, string_ids::menu_mute);
        dropdown::add(1, string_ids::dropdown_without_checkmark, string_ids::menu_play_music);
        dropdown::add(2, 0);
        dropdown::add(3, string_ids::menu_music_options);
        dropdown::show_below(window, widgetIndex, 4, 0);

        if (!audio::isAudioEnabled())
            dropdown::set_item_selected(0);

        if (config::get().music_playing)
            dropdown::set_item_selected(1);

        dropdown::set_highlighted_item(0);
    }

    // 0x0043B0B8
    static void audio_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                audio::toggle_sound();
                break;

            case 1:
            {
                auto& config = config::get();
                if (config.music_playing)
                {
                    config.music_playing = false;
                    audio::stop_background_music();
                }
                else
                {
                    config.music_playing = true;
                }
                config::write();
                break;
            }

            case 3:
                options::open_music_settings();
                break;
        }
    }

    // 0x0043A2B0
    static void railroad_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        // Load objects.
        registers regs;
        regs.edi = (uint32_t)&available_objects[0];
        call(0x004A6841, regs);

        // Sanity check: any objects available?
        uint32_t i = 0;
        while (available_objects[i] != -1 && i < std::size(available_objects))
            i++;
        if (i == 0)
            return;

        auto company_colour = companymgr::get_player_company_colour();

        // Add available objects to dropdown.
        uint16_t highlighted_item = 0;
        for (i = 0; available_objects[i] != -1 && i < std::size(available_objects); i++)
        {
            uint32_t obj_image;
            string_id obj_string_id;

            int objIndex = available_objects[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = objectmgr::get<road_object>(objIndex & 0x7F);
                obj_string_id = road->name;
                obj_image = gfx::recolour(road->var_0E, company_colour);
            }
            else
            {
                auto track = objectmgr::get<track_object>(objIndex);
                obj_string_id = track->name;
                obj_image = gfx::recolour(track->var_1E, company_colour);
            }

            dropdown::add(i, string_ids::menu_sprite_stringid_construction, { obj_image, obj_string_id });

            if (objIndex == last_railroad_option)
                highlighted_item = i;
        }

        dropdown::show_below(window, widgetIndex, i, 25, (1 << 6));
        dropdown::set_highlighted_item(highlighted_item);
    }

    // 0x0043A39F
    static void railroad_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        if (itemIndex == -1)
            return;

        uint8_t objIndex = available_objects[itemIndex];
        construction::openWithFlags(objIndex);
    }

    // 0x0043A965
    static void port_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        uint8_t ddIndex = 0;
        auto interface = objectmgr::get<interface_skin_object>();
        if (addr<0x525FAC, int8_t>() != -1)
        {
            dropdown::add(ddIndex, string_ids::menu_sprite_stringid_construction, { interface->img + interface_skin::image_ids::toolbar_menu_airport, string_ids::menu_airport });
            menu_options[ddIndex] = 0;
            ddIndex++;
        }

        if (addr<0x525FAD, int8_t>() != -1)
        {
            dropdown::add(ddIndex, string_ids::menu_sprite_stringid_construction, { interface->img + interface_skin::image_ids::toolbar_menu_ship_port, string_ids::menu_ship_port });
            menu_options[ddIndex] = 1;
            ddIndex++;
        }

        if (ddIndex == 0)
            return;

        dropdown::show_below(window, widgetIndex, ddIndex, 25, (1 << 6));

        ddIndex = 0;
        if (last_port_option != menu_options[0])
            ddIndex++;

        dropdown::set_highlighted_item(ddIndex);
    }

    // 0x0043AA0A
    static void port_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        last_port_option = menu_options[itemIndex];

        if (last_port_option == 0)
        {
            construction::openWithFlags(1 << 31);
        }
        else if (last_port_option == 1)
        {
            construction::openWithFlags(1 << 30);
        }
    }

    struct VehicleTypeInterfaceParam
    {
        uint32_t image;
        uint32_t build_image;
        string_id build_string;
        string_id num_singular;
        string_id num_plural;
    };

    static const std::map<VehicleType, VehicleTypeInterfaceParam> VehicleTypeInterfaceParameters{
        { VehicleType::bus, { interface_skin::image_ids::vehicle_bus, interface_skin::image_ids::build_vehicle_bus_frame_0, string_ids::build_buses, string_ids::num_buses_singular, string_ids::num_buses_plural } },
        { VehicleType::plane, { interface_skin::image_ids::vehicle_aircraft, interface_skin::image_ids::build_vehicle_aircraft_frame_0, string_ids::build_aircraft, string_ids::num_aircrafts_singular, string_ids::num_aircrafts_plural } },
        { VehicleType::ship, { interface_skin::image_ids::vehicle_ship, interface_skin::image_ids::build_vehicle_ship_frame_0, string_ids::build_ships, string_ids::num_ships_singular, string_ids::num_ships_plural } },
        { VehicleType::train, { interface_skin::image_ids::vehicle_train, interface_skin::image_ids::build_vehicle_train_frame_0, string_ids::build_trains, string_ids::num_trains_singular, string_ids::num_trains_plural } },
        { VehicleType::tram, { interface_skin::image_ids::vehicle_tram, interface_skin::image_ids::build_vehicle_tram_frame_0, string_ids::build_trams, string_ids::num_trams_singular, string_ids::num_trams_plural } },
        { VehicleType::truck, { interface_skin::image_ids::vehicle_truck, interface_skin::image_ids::build_vehicle_truck_frame_0, string_ids::build_trucks, string_ids::num_trucks_singular, string_ids::num_trucks_plural } },
    };

    // 0x0043AD1F
    static void build_vehicles_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto company = companymgr::get(companymgr::get_controlling_id());
        uint16_t available_vehicles = company->available_vehicles;

        auto company_colour = companymgr::get_player_company_colour();
        auto interface = objectmgr::get<interface_skin_object>();

        uint8_t ddIndex = 0;
        for (uint8_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((available_vehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interface_param = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = gfx::recolour(interface_param.build_image, company_colour);

            dropdown::add(ddIndex, string_ids::menu_sprite_stringid, { interface->img + vehicle_image, interface_param.build_string });
            menu_options[ddIndex] = vehicleType;
            ddIndex++;
        }

        dropdown::show_below(window, widgetIndex, ddIndex, 25, (1 << 6));
        dropdown::set_highlighted_item(last_build_vehicles_option);
    }

    // 0x0043ADC7
    static void build_vehicles_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        if (itemIndex == -1)
            return;

        itemIndex = menu_options[itemIndex];
        last_build_vehicles_option = itemIndex;

        build_vehicle::open(itemIndex, 1 << 31);
    }

    // 0x0043ABCB
    static void vehicles_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto player_company_id = companymgr::get_controlling_id();
        auto company = companymgr::get(player_company_id);
        uint16_t available_vehicles = company->available_vehicles;

        auto company_colour = companymgr::get_player_company_colour();
        auto interface = objectmgr::get<interface_skin_object>();

        uint16_t vehicle_counts[vehicleTypeCount]{ 0 };
        for (auto v : thingmgr::VehicleList())
        {
            if (v->owner != player_company_id)
                continue;

            if ((v->var_38 & things::vehicle::flags_38::unk_4) != 0)
                continue;

            vehicle_counts[static_cast<uint8_t>(v->vehicleType)]++;
        }

        uint8_t ddIndex = 0;
        for (uint16_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((available_vehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interface_param = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = gfx::recolour(interface_param.image, company_colour);
            uint16_t vehicle_count = vehicle_counts[vehicleType];

            // TODO: replace with locale-based plurals.
            string_id vehicle_string_id;
            if (vehicle_count == 1)
                vehicle_string_id = interface_param.num_singular;
            else
                vehicle_string_id = interface_param.num_plural;

            dropdown::add(ddIndex, string_ids::menu_sprite_stringid, { interface->img + vehicle_image, vehicle_string_id, vehicle_count });
            menu_options[ddIndex] = vehicleType;
            ddIndex++;
        }

        dropdown::show_below(window, widgetIndex, ddIndex, 25, (1 << 6));
        dropdown::set_highlighted_item(last_vehicles_option);
    }

    // 0x0043ACEF
    static void vehicles_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        if (itemIndex == -1)
            return;

        auto vehicleType = menu_options[itemIndex];
        last_vehicles_option = vehicleType;

        windows::vehicle_list::open(companymgr::get_controlling_id(), vehicleType);
    }

    // 0x0043A4E9
    static void stations_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        uint32_t sprite_base = interface->img;

        // Apply company colour.
        uint32_t company_colour = companymgr::get_player_company_colour();
        sprite_base = gfx::recolour(sprite_base, company_colour);

        dropdown::add(0, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::all_stations, string_ids::all_stations });
        dropdown::add(1, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::rail_stations, string_ids::rail_stations });
        dropdown::add(2, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::road_stations, string_ids::road_stations });
        dropdown::add(3, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::airports, string_ids::airports });
        dropdown::add(4, string_ids::menu_sprite_stringid, { sprite_base + interface_skin::image_ids::ship_ports, string_ids::ship_ports });
        dropdown::show_below(window, widgetIndex, 5, 25, (1 << 6));
        dropdown::set_highlighted_item(0);
    }

    // 0x0043A596
    static void stations_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        if (itemIndex > 4)
            return;

        windows::station_list::open(companymgr::get_controlling_id(), itemIndex);
    }

    // 0x0043A071
    static void on_mouse_down(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case common::widx::loadsave_menu:
                loadsave_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::audio_menu:
                audio_menu_mouse_down(window, widgetIndex);
                break;

            case widx::railroad_menu:
                railroad_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::port_menu:
                port_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::build_vehicles_menu:
                build_vehicles_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::vehicles_menu:
                vehicles_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::stations_menu:
                stations_menu_mouse_down(window, widgetIndex);
                break;

            default:
                common::on_mouse_down(window, widgetIndex);
                break;
        }
    }

    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case common::widx::loadsave_menu:
                loadsave_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::audio_menu:
                audio_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            case widx::railroad_menu:
                railroad_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::port_menu:
                port_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::build_vehicles_menu:
                build_vehicles_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::vehicles_menu:
                vehicles_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::stations_menu:
                stations_menu_dropdown(window, widgetIndex, itemIndex);
                break;

            default:
                common::on_dropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x00439DE4
    static void draw(window* window, gfx::drawpixelinfo_t* dpi)
    {
        common::draw(window, dpi);

        uint32_t company_colour = companymgr::get_player_company_colour();

        if (window->widgets[widx::railroad_menu].type != widget_type::none)
        {
            uint32_t x = window->widgets[widx::railroad_menu].left + window->x;
            uint32_t y = window->widgets[widx::railroad_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = last_railroad_option;
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = objectmgr::get<road_object>(ebx);
                fg_image = gfx::recolour(obj->var_0E, company_colour);
            }
            else
            {
                auto obj = objectmgr::get<track_object>(ebx);
                fg_image = gfx::recolour(obj->var_1E, company_colour);
            }

            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bg_image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_transparent, window->colours[2]);

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = window->widgets[widx::railroad_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        {
            uint32_t x = window->widgets[common::widx::vehicles_menu].left + window->x;
            uint32_t y = window->widgets[common::widx::vehicles_menu].top + window->y;

            static const uint32_t button_face_image_ids[] = {
                interface_skin::image_ids::vehicle_train,
                interface_skin::image_ids::vehicle_bus,
                interface_skin::image_ids::vehicle_truck,
                interface_skin::image_ids::vehicle_tram,
                interface_skin::image_ids::vehicle_aircraft,
                interface_skin::image_ids::vehicle_ship,
            };

            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t fg_image = gfx::recolour(interface->img + button_face_image_ids[last_vehicles_option], company_colour);
            uint32_t bg_image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_transparent, window->colours[3]);

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, common::widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = window->widgets[common::widx::vehicles_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        {
            uint32_t x = window->widgets[common::widx::build_vehicles_menu].left + window->x;
            uint32_t y = window->widgets[common::widx::build_vehicles_menu].top + window->y;

            static const uint32_t build_vehicle_images[] = {
                interface_skin::image_ids::toolbar_build_vehicle_train,
                interface_skin::image_ids::toolbar_build_vehicle_bus,
                interface_skin::image_ids::toolbar_build_vehicle_truck,
                interface_skin::image_ids::toolbar_build_vehicle_tram,
                interface_skin::image_ids::toolbar_build_vehicle_airplane,
                interface_skin::image_ids::toolbar_build_vehicle_boat,
            };

            // Figure out what icon to show on the button face.
            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t fg_image = gfx::recolour(interface->img + build_vehicle_images[last_build_vehicles_option], company_colour);

            if (input::is_dropdown_active(ui::WindowType::topToolbar, common::widx::build_vehicles_menu))
                fg_image++;

            gfx::draw_image(dpi, x, y, fg_image);
        }
    }

    // 0x00439BCB
    static void prepare_draw(window* window)
    {
        auto interface = objectmgr::get<interface_skin_object>();

        if (!audio::isAudioEnabled())
        {
            window->activated_widgets |= (1 << common::widx::audio_menu);
            window->widgets[common::widx::audio_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_audio_inactive, window->colours[0]);
        }
        else
        {
            window->activated_widgets &= ~(1 << common::widx::audio_menu);
            window->widgets[common::widx::audio_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_audio_active, window->colours[0]);
        }

        if (last_port_option == 0 && addr<0x00525FAC, int8_t>() != -1 && addr<0x00525FAD, int8_t>() == -1)
            last_port_option = 1;

        window->widgets[common::widx::loadsave_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_loadsave, 0);
        window->widgets[common::widx::zoom_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_zoom, 0);
        window->widgets[common::widx::rotate_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_rotate, 0);
        window->widgets[common::widx::view_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_view, 0);

        window->widgets[common::widx::terraform_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_terraform, 0);
        window->widgets[widx::railroad_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_opaque, 0);
        window->widgets[common::widx::road_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_opaque, 0);
        window->widgets[common::widx::port_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_opaque, 0);
        window->widgets[common::widx::build_vehicles_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_opaque, 0);

        window->widgets[common::widx::vehicles_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_opaque, 0);
        window->widgets[common::widx::stations_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_stations, 0);

        if (last_town_option == 0)
            window->widgets[common::widx::towns_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_towns, 0);
        else
            window->widgets[common::widx::towns_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_industries, 0);

        if (last_port_option == 0)
            window->widgets[common::widx::port_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_airports, 0);
        else
            window->widgets[common::widx::port_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_ports, 0);

        if (last_road_option != 0xFF)
            window->widgets[common::widx::road_menu].type = widget_type::wt_7;
        else
            window->widgets[common::widx::road_menu].type = widget_type::none;

        if (last_railroad_option != 0xFF)
            window->widgets[widx::railroad_menu].type = widget_type::wt_7;
        else
            window->widgets[widx::railroad_menu].type = widget_type::none;

        if (addr<0x00525FAC, int8_t>() != -1 || addr<0x00525FAD, int8_t>() != -1)
            window->widgets[common::widx::port_menu].type = widget_type::wt_7;
        else
            window->widgets[common::widx::port_menu].type = widget_type::none;

        uint32_t x = std::max(640, ui::width()) - 1;
        common::rightAlignTabs(window, x, { common::widx::towns_menu, common::widx::stations_menu, common::widx::vehicles_menu });
        x -= 11;
        common::rightAlignTabs(window, x, { common::widx::build_vehicles_menu });

        if (window->widgets[common::widx::port_menu].type != widget_type::none)
        {
            common::rightAlignTabs(window, x, { common::widx::port_menu });
        }

        if (window->widgets[common::widx::road_menu].type != widget_type::none)
        {
            common::rightAlignTabs(window, x, { common::widx::road_menu });
        }

        if (window->widgets[widx::railroad_menu].type != widget_type::none)
        {
            common::rightAlignTabs(window, x, { widx::railroad_menu });
        }

        common::rightAlignTabs(window, x, { common::widx::terraform_menu });
    }
}
