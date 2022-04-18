#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Entities/EntityManager.h"
#include "../Game.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../MultiPlayer.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/WaterObject.h"
#include "../S5/S5.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"
#include "ToolbarTopCommon.h"
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Game
{
    static loco_global<uint8_t[40], 0x00113DB20> menu_options;

    static loco_global<uint8_t, 0x00525FAA> last_railroad_option;
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<VehicleType, 0x00525FAF> last_vehicles_option;
    static loco_global<uint8_t, 0x0052622C> last_build_vehicles_option;

    static loco_global<uint32_t, 0x009C86F8> zoom_ticks;

    static loco_global<uint8_t, 0x009C870C> last_town_option;
    static loco_global<uint8_t, 0x009C870D> last_port_option;

    static loco_global<uint8_t[18], 0x0050A006> available_objects;
    // Replaces 0x0050A006
    std::vector<uint8_t> availableTracks;

    namespace Widx
    {
        enum
        {
            cheats_menu = Common::Widx::w2
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::primary),
        makeWidget({ 30, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::primary),
        makeWidget({ 60, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::primary),

        makeWidget({ 104, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::secondary),
        makeWidget({ 134, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::secondary),
        makeWidget({ 164, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::secondary),

        makeWidget({ 267, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary),
        makeWidget({ 387, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary),
        makeWidget({ 357, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary),
        makeWidget({ 417, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary),
        makeWidget({ 417, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary),

        makeWidget({ 490, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::quaternary),
        makeWidget({ 520, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::quaternary),
        makeWidget({ 460, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::quaternary),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseDown(Window* window, WidgetIndex_t widgetIndex);
    static void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    static void prepareDraw(Window* window);
    static void draw(Window* window, Gfx::Context* context);

    // 0x00438B26
    void open()
    {
        zoom_ticks = 0;
        last_town_option = 0;
        last_port_option = 0;

        _events.onResize = Common::onResize;
        _events.event_03 = onMouseDown;
        _events.onMouseDown = onMouseDown;
        _events.onDropdown = onDropdown;
        _events.onUpdate = Common::onUpdate;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            Ui::Size(Ui::width(), 28),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            &_events);
        window->widgets = _widgets;
        window->enabledWidgets = (1 << Common::Widx::loadsave_menu) | (1 << Common::Widx::audio_menu) | (1 << Common::Widx::zoom_menu) | (1 << Common::Widx::rotate_menu) | (1 << Common::Widx::view_menu) | (1 << Common::Widx::terraform_menu) | (1 << Common::Widx::railroad_menu) | (1 << Common::Widx::road_menu) | (1 << Common::Widx::port_menu) | (1 << Common::Widx::build_vehicles_menu) | (1 << Common::Widx::vehicles_menu) | (1 << Common::Widx::stations_menu) | (1 << Common::Widx::towns_menu);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, skin->colour_12);
            window->setColour(WindowColour::secondary, skin->colour_13);
            window->setColour(WindowColour::tertiary, skin->colour_14);
            window->setColour(WindowColour::quaternary, skin->colour_15);
        }
    }

    // 0x0043B0F7
    static void loadsaveMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::menu_load_game);
        Dropdown::add(1, StringIds::menu_save_game);
        Dropdown::add(2, 0);
        Dropdown::add(3, StringIds::menu_about);
        Dropdown::add(4, StringIds::options);
        Dropdown::add(5, StringIds::menu_screenshot);
        Dropdown::add(6, 0);
        Dropdown::add(7, StringIds::menu_quit_to_menu);
        Dropdown::add(8, StringIds::menu_exit_openloco);
        Dropdown::showBelow(window, widgetIndex, 9, 0);
        Dropdown::setHighlightedItem(1);
    }

    // 0x0043B1C4
    static void prepareSaveGame()
    {
        Input::toolCancel();

        if (isNetworked())
        {
            if (CompanyManager::getUpdatingCompanyId() == CompanyManager::getControllingId())
            {
                GameCommands::do_72();
                MultiPlayer::setFlag(MultiPlayer::flags::flag_2);
            }
            return;
        }

        if (!OpenLoco::Game::saveSaveGameOpen())
        {
            // Cancelled by user
            Gfx::invalidateScreen();
            return;
        }

        static loco_global<char[512], 0x0112CE04> _savePath;
        auto path = fs::u8path(&_savePath[0]).replace_extension(S5::extensionSV5);

        // Store path to active file
        static loco_global<char[256], 0x0050B745> _currentGameFilePath;
        strncpy(&_currentGameFilePath[0], path.u8string().c_str(), std::size(_currentGameFilePath));

        uint32_t flags = 0;
        if (Config::get().flags & Config::Flags::exportObjectsWithSaves)
            flags = S5::SaveFlags::packCustomObjects;

        if (!S5::save(path, flags))
            Error::open(StringIds::error_game_save_failed, StringIds::null);
    }

    // 0x0043B154
    static void loadsaveMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                // Load game
                GameCommands::do_21(0, 0);
                break;

            case 1:
                // Save game
                prepareSaveGame();
                break;

            case 3:
                About::open();
                break;

            case 4:
                Options::open();
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
    static void audioMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_mute);
        Dropdown::add(1, StringIds::dropdown_without_checkmark, StringIds::menu_play_music);
        Dropdown::add(2, 0);
        Dropdown::add(3, StringIds::menu_music_options);
        Dropdown::showBelow(window, widgetIndex, 4, 0);

        if (!Audio::isAudioEnabled())
            Dropdown::setItemSelected(0);

        if (Config::get().musicPlaying)
            Dropdown::setItemSelected(1);

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043B0B8
    static void audioMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                Audio::toggleSound();
                break;

            case 1:
            {
                auto& config = Config::get();
                if (config.musicPlaying)
                {
                    config.musicPlaying = false;
                    Audio::stopBackgroundMusic();
                }
                else
                {
                    config.musicPlaying = true;
                }
                Config::write();
                break;
            }

            case 3:
                Options::openMusicSettings();
                break;
        }
    }

    static void cheatsMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::cheats);
        Dropdown::add(1, StringIds::tile_inspector);
        Dropdown::add(2, 0);
        Dropdown::add(3, StringIds::dropdown_without_checkmark, StringIds::cheat_enable_sandbox_mode);
        Dropdown::add(4, StringIds::dropdown_without_checkmark, StringIds::cheat_allow_building_while_paused);
        Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::cheat_allow_manual_driving);

        Dropdown::showBelow(window, widgetIndex, 6, 0);

        if (isSandboxMode())
            Dropdown::setItemSelected(3);

        if (isPauseOverrideEnabled())
            Dropdown::setItemSelected(4);

        if (isDriverCheatEnabled())
            Dropdown::setItemSelected(5);
    }

    static void cheatsMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = 0;

        switch (itemIndex)
        {
            case 0:
                Cheats::open();
                break;

            case 1:
                TileInspector::open();
                break;

            case 3:
                if (!isSandboxMode())
                    setScreenFlag(ScreenFlags::sandboxMode);
                else
                    clearScreenFlag(ScreenFlags::sandboxMode);
                break;

            case 4:
                if (!isPauseOverrideEnabled())
                    setScreenFlag(ScreenFlags::pauseOverrideEnabled);
                else
                    clearScreenFlag(ScreenFlags::pauseOverrideEnabled);
                break;

            case 5:
                if (!isDriverCheatEnabled())
                    setScreenFlag(ScreenFlags::driverCheatEnabled);
                else
                    clearScreenFlag(ScreenFlags::driverCheatEnabled);
                break;
        }
    }

    // 0x0043A2B0
    static void railroadMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        // Load dropdown objects removing any that are not unlocked.
        // Note: This is not using player company id! This looks odd.
        availableTracks = CompanyManager::get(GameCommands::getUpdatingCompanyId())->getAvailableRailTracks();

        assert(std::size(available_objects) >= std::size(availableTracks));
        // Legacy copy to available_objects remove when all users of 0x0050A006 accounted for
        std::copy(std::begin(availableTracks), std::end(availableTracks), std::begin(available_objects));
        available_objects[availableTracks.size()] = std::numeric_limits<uint8_t>::max();

        if (availableTracks.size() == 0)
            return;

        auto companyColour = CompanyManager::getPlayerCompanyColour();

        // Add available objects to Dropdown.
        uint16_t highlighted_item = 0;

        for (auto i = 0u; i < std::size(availableTracks); i++)
        {
            uint32_t obj_image;
            string_id obj_string_id;

            auto objIndex = availableTracks[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = ObjectManager::get<RoadObject>(objIndex & 0x7F);
                obj_string_id = road->name;
                obj_image = Gfx::recolour(road->image, companyColour);
            }
            else
            {
                auto track = ObjectManager::get<TrackObject>(objIndex);
                obj_string_id = track->name;
                obj_image = Gfx::recolour(track->image, companyColour);
            }

            Dropdown::add(i, StringIds::menu_sprite_stringid_construction, { obj_image, obj_string_id });

            if (objIndex == last_railroad_option)
                highlighted_item = i;
        }

        Dropdown::showBelow(window, widgetIndex, std::size(availableTracks), 25, (1 << 6));
        Dropdown::setHighlightedItem(highlighted_item);
    }

    // 0x0043A39F
    static void railroadMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        uint8_t objIndex = availableTracks[itemIndex];
        Construction::openWithFlags(objIndex);
    }

    // 0x0043A965
    static void portMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        uint8_t ddIndex = 0;
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        if (addr<0x525FAC, int8_t>() != -1)
        {
            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_airport, StringIds::menu_airport });
            menu_options[ddIndex] = 0;
            ddIndex++;
        }

        if (addr<0x525FAD, int8_t>() != -1)
        {
            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_ship_port, StringIds::menu_ship_port });
            menu_options[ddIndex] = 1;
            ddIndex++;
        }

        if (ddIndex == 0)
            return;

        Dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));

        ddIndex = 0;
        if (last_port_option != menu_options[0])
            ddIndex++;

        Dropdown::setHighlightedItem(ddIndex);
    }

    // 0x0043AA0A
    static void portMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        last_port_option = menu_options[itemIndex];

        if (last_port_option == 0)
        {
            Construction::openWithFlags(1 << 31);
        }
        else if (last_port_option == 1)
        {
            Construction::openWithFlags(1 << 30);
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

    // clang-format off
    static const std::map<VehicleType, VehicleTypeInterfaceParam> VehicleTypeInterfaceParameters{
        { VehicleType::bus,      { InterfaceSkin::ImageIds::vehicle_buses_frame_0,      InterfaceSkin::ImageIds::build_vehicle_bus_frame_0,      StringIds::build_buses,    StringIds::num_buses_singular,     StringIds::num_buses_plural } },
        { VehicleType::aircraft, { InterfaceSkin::ImageIds::vehicle_aircraft_frame_0, InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_0, StringIds::build_aircraft, StringIds::num_aircrafts_singular, StringIds::num_aircrafts_plural } },
        { VehicleType::ship,     { InterfaceSkin::ImageIds::vehicle_ships_frame_0,     InterfaceSkin::ImageIds::build_vehicle_ship_frame_0,     StringIds::build_ships,    StringIds::num_ships_singular,     StringIds::num_ships_plural } },
        { VehicleType::train,    { InterfaceSkin::ImageIds::vehicle_train_frame_0,    InterfaceSkin::ImageIds::build_vehicle_train_frame_0,    StringIds::build_trains,   StringIds::num_trains_singular,    StringIds::num_trains_plural } },
        { VehicleType::tram,     { InterfaceSkin::ImageIds::vehicle_trams_frame_0,     InterfaceSkin::ImageIds::build_vehicle_tram_frame_0,     StringIds::build_trams,    StringIds::num_trams_singular,     StringIds::num_trams_plural } },
        { VehicleType::truck,    { InterfaceSkin::ImageIds::vehicle_trucks_frame_0,    InterfaceSkin::ImageIds::build_vehicle_truck_frame_0,    StringIds::build_trucks,   StringIds::num_trucks_singular,    StringIds::num_trucks_plural } },
    };
    // clang-format on

    // 0x0043AD1F
    static void buildVehiclesMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto company = CompanyManager::get(CompanyManager::getControllingId());
        uint16_t availableVehicles = company->availableVehicles;

        auto companyColour = CompanyManager::getPlayerCompanyColour();
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        uint8_t ddIndex = 0;
        for (uint8_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((availableVehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interface_param = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = Gfx::recolour(interface_param.build_image, companyColour);

            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicle_image, interface_param.build_string });
            menu_options[ddIndex] = vehicleType;
            ddIndex++;
        }

        Dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));
        Dropdown::setHighlightedItem(last_build_vehicles_option);
    }

    // 0x0043ADC7
    static void buildVehiclesMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        itemIndex = menu_options[itemIndex];
        last_build_vehicles_option = itemIndex;

        BuildVehicle::open(itemIndex, 1 << 31);
    }

    // 0x0043ABCB
    static void vehiclesMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto player_company_id = CompanyManager::getControllingId();
        auto company = CompanyManager::get(player_company_id);
        uint16_t availableVehicles = company->availableVehicles;

        auto companyColour = CompanyManager::getPlayerCompanyColour();
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        uint16_t vehicle_counts[vehicleTypeCount]{ 0 };
        for (auto v : EntityManager::VehicleList())
        {
            if (v->owner != player_company_id)
                continue;

            if ((v->var_38 & Vehicles::Flags38::isGhost) != 0)
                continue;

            vehicle_counts[static_cast<uint8_t>(v->vehicleType)]++;
        }

        uint8_t ddIndex = 0;
        for (uint16_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((availableVehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interface_param = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = Gfx::recolour(interface_param.image, companyColour);
            uint16_t vehicle_count = vehicle_counts[vehicleType];

            // TODO: replace with locale-based plurals.
            string_id vehicle_string_id;
            if (vehicle_count == 1)
                vehicle_string_id = interface_param.num_singular;
            else
                vehicle_string_id = interface_param.num_plural;

            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicle_image, vehicle_string_id, vehicle_count });
            menu_options[ddIndex] = vehicleType;
            ddIndex++;
        }

        Dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));
        Dropdown::setHighlightedItem(static_cast<uint8_t>(*last_vehicles_option));
    }

    // 0x0043ACEF
    static void vehiclesMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        auto vehicleType = VehicleType(menu_options[itemIndex]);
        last_vehicles_option = vehicleType;

        VehicleList::open(CompanyManager::getControllingId(), vehicleType);
    }

    // 0x0043A4E9
    static void stationsMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        uint32_t sprite_base = interface->img;

        // Apply company colour.
        const auto companyColour = CompanyManager::getPlayerCompanyColour();
        sprite_base = Gfx::recolour(sprite_base, companyColour);

        Dropdown::add(0, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::all_stations, StringIds::all_stations });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::rail_stations, StringIds::rail_stations });
        Dropdown::add(2, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::road_stations, StringIds::road_stations });
        Dropdown::add(3, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::airports, StringIds::airports });
        Dropdown::add(4, StringIds::menu_sprite_stringid, { sprite_base + InterfaceSkin::ImageIds::ship_ports, StringIds::ship_ports });
        Dropdown::showBelow(window, widgetIndex, 5, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A596
    static void stationsMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex > 4)
            return;

        StationList::open(CompanyManager::getControllingId(), itemIndex);
    }

    // 0x0043A071
    static void onMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuMouseDown(window, widgetIndex);
                break;

            case Widx::cheats_menu:
                cheatsMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::railroad_menu:
                railroadMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::port_menu:
                portMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::build_vehicles_menu:
                buildVehiclesMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::vehicles_menu:
                vehiclesMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::stations_menu:
                stationsMenuMouseDown(window, widgetIndex);
                break;

            default:
                Common::onMouseDown(window, widgetIndex);
                break;
        }
    }

    static void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::cheats_menu:
                cheatsMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::railroad_menu:
                railroadMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::port_menu:
                portMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::build_vehicles_menu:
                buildVehiclesMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::vehicles_menu:
                vehiclesMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::stations_menu:
                stationsMenuDropdown(window, widgetIndex, itemIndex);
                break;

            default:
                Common::onDropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x00439DE4
    static void draw(Window* window, Gfx::Context* context)
    {
        Common::draw(window, context);

        const auto companyColour = CompanyManager::getPlayerCompanyColour();

        if (window->widgets[Common::Widx::railroad_menu].type != WidgetType::none)
        {
            uint32_t x = window->widgets[Common::Widx::railroad_menu].left + window->x;
            uint32_t y = window->widgets[Common::Widx::railroad_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = last_railroad_option;
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = ObjectManager::get<RoadObject>(ebx);
                fg_image = Gfx::recolour(obj->image, companyColour);
            }
            else
            {
                auto obj = ObjectManager::get<TrackObject>(ebx);
                fg_image = Gfx::recolour(obj->image, companyColour);
            }

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, window->getColour(WindowColour::tertiary).c());

            y--;
            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Common::Widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            Gfx::drawImage(context, x, y, fg_image);

            y = window->widgets[Common::Widx::railroad_menu].top + window->y;
            Gfx::drawImage(context, x, y, bg_image);
        }

        {
            uint32_t x = window->widgets[Common::Widx::vehicles_menu].left + window->x;
            uint32_t y = window->widgets[Common::Widx::vehicles_menu].top + window->y;

            static const uint32_t button_face_image_ids[] = {
                InterfaceSkin::ImageIds::vehicle_train_frame_0,
                InterfaceSkin::ImageIds::vehicle_buses_frame_0,
                InterfaceSkin::ImageIds::vehicle_trucks_frame_0,
                InterfaceSkin::ImageIds::vehicle_trams_frame_0,
                InterfaceSkin::ImageIds::vehicle_aircraft_frame_0,
                InterfaceSkin::ImageIds::vehicle_ships_frame_0,
            };

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t fg_image = Gfx::recolour(interface->img + button_face_image_ids[static_cast<uint8_t>(*last_vehicles_option)], companyColour);
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, window->getColour(WindowColour::quaternary).c());

            y--;
            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Common::Widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            Gfx::drawImage(context, x, y, fg_image);

            y = window->widgets[Common::Widx::vehicles_menu].top + window->y;
            Gfx::drawImage(context, x, y, bg_image);
        }

        {
            uint32_t x = window->widgets[Common::Widx::build_vehicles_menu].left + window->x;
            uint32_t y = window->widgets[Common::Widx::build_vehicles_menu].top + window->y;

            static const uint32_t build_vehicle_images[] = {
                InterfaceSkin::ImageIds::toolbar_build_vehicle_train,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_bus,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_truck,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_tram,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_airplane,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_boat,
            };

            // Figure out what icon to show on the button face.
            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t fg_image = Gfx::recolour(interface->img + build_vehicle_images[last_build_vehicles_option], companyColour);

            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Common::Widx::build_vehicles_menu))
                fg_image++;

            Gfx::drawImage(context, x, y, fg_image);
        }
    }

    // 0x00439BCB
    static void prepareDraw(Window* window)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        if (!Audio::isAudioEnabled())
        {
            window->activatedWidgets |= (1 << Common::Widx::audio_menu);
            window->widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, window->getColour(WindowColour::primary).c());
        }
        else
        {
            window->activatedWidgets &= ~(1 << Common::Widx::audio_menu);
            window->widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, window->getColour(WindowColour::primary).c());
        }

        if (Config::getNew().cheatsMenuEnabled)
        {
            window->widgets[Widx::cheats_menu].type = WidgetType::toolbarTab;
            auto& baseWidget = window->widgets[Widx::cheats_menu];
            window->widgets[Common::Widx::zoom_menu].left = baseWidget.left + 14 + (baseWidget.width() * 1);
            window->widgets[Common::Widx::rotate_menu].left = baseWidget.left + 14 + (baseWidget.width() * 2);
            window->widgets[Common::Widx::view_menu].left = baseWidget.left + 14 + (baseWidget.width() * 3);
        }
        else
        {
            window->widgets[Widx::cheats_menu].type = WidgetType::none;
            auto& baseWidget = window->widgets[Common::Widx::audio_menu];
            window->widgets[Common::Widx::zoom_menu].left = baseWidget.left + 14 + (baseWidget.width() * 1);
            window->widgets[Common::Widx::rotate_menu].left = baseWidget.left + 14 + (baseWidget.width() * 2);
            window->widgets[Common::Widx::view_menu].left = baseWidget.left + 14 + (baseWidget.width() * 3);
        }

        if (last_port_option == 0 && addr<0x00525FAC, int8_t>() != -1 && addr<0x00525FAD, int8_t>() == -1)
            last_port_option = 1;

        window->widgets[Common::Widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave);
        window->widgets[Widx::cheats_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_cogwheels);
        window->widgets[Common::Widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom);
        window->widgets[Common::Widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate);
        window->widgets[Common::Widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view);

        window->widgets[Common::Widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform);
        window->widgets[Common::Widx::railroad_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window->widgets[Common::Widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window->widgets[Common::Widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window->widgets[Common::Widx::build_vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);

        window->widgets[Common::Widx::vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window->widgets[Common::Widx::stations_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_stations);

        if (last_town_option == 0)
            window->widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns);
        else
            window->widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries);

        if (last_port_option == 0)
            window->widgets[Common::Widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_airports);
        else
            window->widgets[Common::Widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_ports);

        if (last_road_option != 0xFF)
            window->widgets[Common::Widx::road_menu].type = WidgetType::toolbarTab;
        else
            window->widgets[Common::Widx::road_menu].type = WidgetType::none;

        if (last_railroad_option != 0xFF)
            window->widgets[Common::Widx::railroad_menu].type = WidgetType::toolbarTab;
        else
            window->widgets[Common::Widx::railroad_menu].type = WidgetType::none;

        if (addr<0x00525FAC, int8_t>() != -1 || addr<0x00525FAD, int8_t>() != -1)
            window->widgets[Common::Widx::port_menu].type = WidgetType::toolbarTab;
        else
            window->widgets[Common::Widx::port_menu].type = WidgetType::none;

        uint32_t x = std::max(640, Ui::width()) - 1;
        Common::rightAlignTabs(window, x, { Common::Widx::towns_menu, Common::Widx::stations_menu, Common::Widx::vehicles_menu });
        x -= 11;
        Common::rightAlignTabs(window, x, { Common::Widx::build_vehicles_menu });

        if (window->widgets[Common::Widx::port_menu].type != WidgetType::none)
        {
            Common::rightAlignTabs(window, x, { Common::Widx::port_menu });
        }

        if (window->widgets[Common::Widx::road_menu].type != WidgetType::none)
        {
            Common::rightAlignTabs(window, x, { Common::Widx::road_menu });
        }

        if (window->widgets[Common::Widx::railroad_menu].type != WidgetType::none)
        {
            Common::rightAlignTabs(window, x, { Common::Widx::railroad_menu });
        }

        Common::rightAlignTabs(window, x, { Common::Widx::terraform_menu });
    }
}
