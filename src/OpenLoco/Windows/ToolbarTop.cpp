#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/WaterObject.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../Things/Vehicle.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "ToolbarTopCommon.h"
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::ui::windows::toolbar_top::game
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
        makeWidget({ 0, 0 }, { 30, 28 }, widget_type::wt_7, 0),
        makeWidget({ 30, 0 }, { 30, 28 }, widget_type::wt_7, 0),
        makeWidget({ 74, 0 }, { 30, 28 }, widget_type::wt_7, 1),
        makeWidget({ 104, 0 }, { 30, 28 }, widget_type::wt_7, 1),
        makeWidget({ 134, 0 }, { 30, 28 }, widget_type::wt_7, 1),

        makeWidget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        makeWidget({ 387, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        makeWidget({ 357, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        makeWidget({ 417, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        makeWidget({ 417, 0 }, { 30, 28 }, widget_type::wt_7, 2),

        makeWidget({ 490, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        makeWidget({ 520, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        makeWidget({ 460, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        widgetEnd(),
    };

    static window_event_list _events;

    static void onMouseDown(window* window, widget_index widgetIndex);
    static void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    static void prepareDraw(window* window);
    static void draw(window* window, Gfx::drawpixelinfo_t* dpi);

    // 0x00438B26
    void open()
    {
        zoom_ticks = 0;
        last_town_option = 0;
        last_port_option = 0;

        _events.on_resize = common::onResize;
        _events.event_03 = onMouseDown;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;
        _events.on_update = common::onUpdate;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            Gfx::ui_size_t(ui::width(), 28),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << common::widx::loadsave_menu) | (1 << common::widx::audio_menu) | (1 << common::widx::zoom_menu) | (1 << common::widx::rotate_menu) | (1 << common::widx::view_menu) | (1 << common::widx::terraform_menu) | (1 << widx::railroad_menu) | (1 << common::widx::road_menu) | (1 << common::widx::port_menu) | (1 << common::widx::build_vehicles_menu) | (1 << common::widx::vehicles_menu) | (1 << common::widx::stations_menu) | (1 << common::widx::towns_menu);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = skin->colour_12;
            window->colours[1] = skin->colour_13;
            window->colours[2] = skin->colour_14;
            window->colours[3] = skin->colour_15;
        }
    }

    // 0x0043B0F7
    static void loadsaveMenuMouseDown(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, StringIds::menu_load_game);
        dropdown::add(1, StringIds::menu_save_game);
        dropdown::add(2, 0);
        dropdown::add(3, StringIds::menu_about);
        dropdown::add(4, StringIds::options);
        dropdown::add(5, StringIds::menu_screenshot);
        dropdown::add(6, 0);
        dropdown::add(7, StringIds::menu_quit_to_menu);
        dropdown::add(8, StringIds::menu_exit_openloco);
        dropdown::showBelow(window, widgetIndex, 9, 0);
        dropdown::setHighlightedItem(1);
    }

    // 0x0043B154
    static void loadsaveMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                // Load game
                GameCommands::do_21(0, 0);
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
                GameCommands::do_21(0, 1);
                break;

            case 8:
                // Exit to desktop
                GameCommands::do_21(0, 2);
                break;
        }
    }

    // 0x0043B04B
    static void audioMenuMouseDown(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_mute);
        dropdown::add(1, StringIds::dropdown_without_checkmark, StringIds::menu_play_music);
        dropdown::add(2, 0);
        dropdown::add(3, StringIds::menu_music_options);
        dropdown::showBelow(window, widgetIndex, 4, 0);

        if (!Audio::isAudioEnabled())
            dropdown::setItemSelected(0);

        if (config::get().music_playing)
            dropdown::setItemSelected(1);

        dropdown::setHighlightedItem(0);
    }

    // 0x0043B0B8
    static void audioMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                Audio::toggleSound();
                break;

            case 1:
            {
                auto& config = config::get();
                if (config.music_playing)
                {
                    config.music_playing = false;
                    Audio::stopBackgroundMusic();
                }
                else
                {
                    config.music_playing = true;
                }
                config::write();
                break;
            }

            case 3:
                options::openMusicSettings();
                break;
        }
    }

    // 0x0043A2B0
    static void railroadMenuMouseDown(window* window, widget_index widgetIndex)
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

        auto company_colour = companymgr::getPlayerCompanyColour();

        // Add available objects to dropdown.
        uint16_t highlighted_item = 0;
        for (i = 0; available_objects[i] != -1 && i < std::size(available_objects); i++)
        {
            uint32_t obj_image;
            string_id obj_string_id;

            int objIndex = available_objects[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = ObjectManager::get<road_object>(objIndex & 0x7F);
                obj_string_id = road->name;
                obj_image = Gfx::recolour(road->var_0E, company_colour);
            }
            else
            {
                auto track = ObjectManager::get<track_object>(objIndex);
                obj_string_id = track->name;
                obj_image = Gfx::recolour(track->var_1E, company_colour);
            }

            dropdown::add(i, StringIds::menu_sprite_stringid_construction, { obj_image, obj_string_id });

            if (objIndex == last_railroad_option)
                highlighted_item = i;
        }

        dropdown::showBelow(window, widgetIndex, i, 25, (1 << 6));
        dropdown::setHighlightedItem(highlighted_item);
    }

    // 0x0043A39F
    static void railroadMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        uint8_t objIndex = available_objects[itemIndex];
        construction::openWithFlags(objIndex);
    }

    // 0x0043A965
    static void portMenuMouseDown(window* window, widget_index widgetIndex)
    {
        uint8_t ddIndex = 0;
        auto interface = ObjectManager::get<interface_skin_object>();
        if (addr<0x525FAC, int8_t>() != -1)
        {
            dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_airport, StringIds::menu_airport });
            menu_options[ddIndex] = 0;
            ddIndex++;
        }

        if (addr<0x525FAD, int8_t>() != -1)
        {
            dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_ship_port, StringIds::menu_ship_port });
            menu_options[ddIndex] = 1;
            ddIndex++;
        }

        if (ddIndex == 0)
            return;

        dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));

        ddIndex = 0;
        if (last_port_option != menu_options[0])
            ddIndex++;

        dropdown::setHighlightedItem(ddIndex);
    }

    // 0x0043AA0A
    static void portMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

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
        { VehicleType::bus, { InterfaceSkin::ImageIds::vehicle_bus, InterfaceSkin::ImageIds::build_vehicle_bus_frame_0, StringIds::build_buses, StringIds::num_buses_singular, StringIds::num_buses_plural } },
        { VehicleType::plane, { InterfaceSkin::ImageIds::vehicle_aircraft, InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_0, StringIds::build_aircraft, StringIds::num_aircrafts_singular, StringIds::num_aircrafts_plural } },
        { VehicleType::ship, { InterfaceSkin::ImageIds::vehicle_ship, InterfaceSkin::ImageIds::build_vehicle_ship_frame_0, StringIds::build_ships, StringIds::num_ships_singular, StringIds::num_ships_plural } },
        { VehicleType::train, { InterfaceSkin::ImageIds::vehicle_train_frame_0, InterfaceSkin::ImageIds::build_vehicle_train_frame_0, StringIds::build_trains, StringIds::num_trains_singular, StringIds::num_trains_plural } },
        { VehicleType::tram, { InterfaceSkin::ImageIds::vehicle_tram, InterfaceSkin::ImageIds::build_vehicle_tram_frame_0, StringIds::build_trams, StringIds::num_trams_singular, StringIds::num_trams_plural } },
        { VehicleType::truck, { InterfaceSkin::ImageIds::vehicle_truck, InterfaceSkin::ImageIds::build_vehicle_truck_frame_0, StringIds::build_trucks, StringIds::num_trucks_singular, StringIds::num_trucks_plural } },
    };

    // 0x0043AD1F
    static void buildVehiclesMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto company = companymgr::get(companymgr::getControllingId());
        uint16_t available_vehicles = company->available_vehicles;

        auto company_colour = companymgr::getPlayerCompanyColour();
        auto interface = ObjectManager::get<interface_skin_object>();

        uint8_t ddIndex = 0;
        for (uint8_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((available_vehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interface_param = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = Gfx::recolour(interface_param.build_image, company_colour);

            dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicle_image, interface_param.build_string });
            menu_options[ddIndex] = vehicleType;
            ddIndex++;
        }

        dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));
        dropdown::setHighlightedItem(last_build_vehicles_option);
    }

    // 0x0043ADC7
    static void buildVehiclesMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        itemIndex = menu_options[itemIndex];
        last_build_vehicles_option = itemIndex;

        BuildVehicle::open(itemIndex, 1 << 31);
    }

    // 0x0043ABCB
    static void vehiclesMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto player_company_id = companymgr::getControllingId();
        auto company = companymgr::get(player_company_id);
        uint16_t available_vehicles = company->available_vehicles;

        auto company_colour = companymgr::getPlayerCompanyColour();
        auto interface = ObjectManager::get<interface_skin_object>();

        uint16_t vehicle_counts[vehicleTypeCount]{ 0 };
        for (auto v : thingmgr::VehicleList())
        {
            if (v->owner != player_company_id)
                continue;

            if ((v->var_38 & Things::vehicle::flags_38::unk_4) != 0)
                continue;

            vehicle_counts[static_cast<uint8_t>(v->vehicleType)]++;
        }

        uint8_t ddIndex = 0;
        for (uint16_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((available_vehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interface_param = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = Gfx::recolour(interface_param.image, company_colour);
            uint16_t vehicle_count = vehicle_counts[vehicleType];

            // TODO: replace with locale-based plurals.
            string_id vehicle_string_id;
            if (vehicle_count == 1)
                vehicle_string_id = interface_param.num_singular;
            else
                vehicle_string_id = interface_param.num_plural;

            dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicle_image, vehicle_string_id, vehicle_count });
            menu_options[ddIndex] = vehicleType;
            ddIndex++;
        }

        dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));
        dropdown::setHighlightedItem(last_vehicles_option);
    }

    // 0x0043ACEF
    static void vehiclesMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        auto vehicleType = menu_options[itemIndex];
        last_vehicles_option = vehicleType;

        windows::vehicle_list::open(companymgr::getControllingId(), vehicleType);
    }

    // 0x0043A4E9
    static void stationsMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = ObjectManager::get<interface_skin_object>();
        uint32_t sprite_base = interface->img;

        // Apply company colour.
        uint32_t company_colour = companymgr::getPlayerCompanyColour();
        sprite_base = Gfx::recolour(sprite_base, company_colour);

        dropdown::add(0, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::all_stations, StringIds::all_stations });
        dropdown::add(1, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::rail_stations, StringIds::rail_stations });
        dropdown::add(2, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::road_stations, StringIds::road_stations });
        dropdown::add(3, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::airports, StringIds::airports });
        dropdown::add(4, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::ship_ports, StringIds::ship_ports });
        dropdown::showBelow(window, widgetIndex, 5, 25, (1 << 6));
        dropdown::setHighlightedItem(0);
    }

    // 0x0043A596
    static void stationsMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (itemIndex > 4)
            return;

        windows::station_list::open(companymgr::getControllingId(), itemIndex);
    }

    // 0x0043A071
    static void onMouseDown(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case common::widx::loadsave_menu:
                loadsaveMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::audio_menu:
                audioMenuMouseDown(window, widgetIndex);
                break;

            case widx::railroad_menu:
                railroadMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::port_menu:
                portMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::build_vehicles_menu:
                buildVehiclesMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::vehicles_menu:
                vehiclesMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::stations_menu:
                stationsMenuMouseDown(window, widgetIndex);
                break;

            default:
                common::onMouseDown(window, widgetIndex);
                break;
        }
    }

    static void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case common::widx::loadsave_menu:
                loadsaveMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::audio_menu:
                audioMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case widx::railroad_menu:
                railroadMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::port_menu:
                portMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::build_vehicles_menu:
                buildVehiclesMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::vehicles_menu:
                vehiclesMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::stations_menu:
                stationsMenuDropdown(window, widgetIndex, itemIndex);
                break;

            default:
                common::onDropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x00439DE4
    static void draw(window* window, Gfx::drawpixelinfo_t* dpi)
    {
        common::draw(window, dpi);

        uint32_t company_colour = companymgr::getPlayerCompanyColour();

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
                auto obj = ObjectManager::get<road_object>(ebx);
                fg_image = Gfx::recolour(obj->var_0E, company_colour);
            }
            else
            {
                auto obj = ObjectManager::get<track_object>(ebx);
                fg_image = Gfx::recolour(obj->var_1E, company_colour);
            }

            auto interface = ObjectManager::get<interface_skin_object>();
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, window->colours[2]);

            y--;
            if (Input::isDropdownActive(ui::WindowType::topToolbar, widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            Gfx::drawImage(dpi, x, y, fg_image);

            y = window->widgets[widx::railroad_menu].top + window->y;
            Gfx::drawImage(dpi, x, y, bg_image);
        }

        {
            uint32_t x = window->widgets[common::widx::vehicles_menu].left + window->x;
            uint32_t y = window->widgets[common::widx::vehicles_menu].top + window->y;

            static const uint32_t button_face_image_ids[] = {
                InterfaceSkin::ImageIds::vehicle_train_frame_0,
                InterfaceSkin::ImageIds::vehicle_bus,
                InterfaceSkin::ImageIds::vehicle_truck,
                InterfaceSkin::ImageIds::vehicle_tram,
                InterfaceSkin::ImageIds::vehicle_aircraft,
                InterfaceSkin::ImageIds::vehicle_ship,
            };

            auto interface = ObjectManager::get<interface_skin_object>();
            uint32_t fg_image = Gfx::recolour(interface->img + button_face_image_ids[last_vehicles_option], company_colour);
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, window->colours[3]);

            y--;
            if (Input::isDropdownActive(ui::WindowType::topToolbar, common::widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            Gfx::drawImage(dpi, x, y, fg_image);

            y = window->widgets[common::widx::vehicles_menu].top + window->y;
            Gfx::drawImage(dpi, x, y, bg_image);
        }

        {
            uint32_t x = window->widgets[common::widx::build_vehicles_menu].left + window->x;
            uint32_t y = window->widgets[common::widx::build_vehicles_menu].top + window->y;

            static const uint32_t build_vehicle_images[] = {
                InterfaceSkin::ImageIds::toolbar_build_vehicle_train,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_bus,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_truck,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_tram,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_airplane,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_boat,
            };

            // Figure out what icon to show on the button face.
            auto interface = ObjectManager::get<interface_skin_object>();
            uint32_t fg_image = Gfx::recolour(interface->img + build_vehicle_images[last_build_vehicles_option], company_colour);

            if (Input::isDropdownActive(ui::WindowType::topToolbar, common::widx::build_vehicles_menu))
                fg_image++;

            Gfx::drawImage(dpi, x, y, fg_image);
        }
    }

    // 0x00439BCB
    static void prepareDraw(window* window)
    {
        auto interface = ObjectManager::get<interface_skin_object>();

        if (!Audio::isAudioEnabled())
        {
            window->activated_widgets |= (1 << common::widx::audio_menu);
            window->widgets[common::widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, window->colours[0]);
        }
        else
        {
            window->activated_widgets &= ~(1 << common::widx::audio_menu);
            window->widgets[common::widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, window->colours[0]);
        }

        if (last_port_option == 0 && addr<0x00525FAC, int8_t>() != -1 && addr<0x00525FAD, int8_t>() == -1)
            last_port_option = 1;

        window->widgets[common::widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave, 0);
        window->widgets[common::widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom, 0);
        window->widgets[common::widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate, 0);
        window->widgets[common::widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view, 0);

        window->widgets[common::widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform, 0);
        window->widgets[widx::railroad_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque, 0);
        window->widgets[common::widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque, 0);
        window->widgets[common::widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque, 0);
        window->widgets[common::widx::build_vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque, 0);

        window->widgets[common::widx::vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque, 0);
        window->widgets[common::widx::stations_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_stations, 0);

        if (last_town_option == 0)
            window->widgets[common::widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns, 0);
        else
            window->widgets[common::widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries, 0);

        if (last_port_option == 0)
            window->widgets[common::widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_airports, 0);
        else
            window->widgets[common::widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_ports, 0);

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
