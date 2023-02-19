#include "Audio/Audio.h"
#include "CompanyManager.h"
#include "Config.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "LastGameOptionManager.h"
#include "Localisation/StringIds.h"
#include "MultiPlayer.h"
#include "Network/Network.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "StationManager.h"
#include "ToolbarTopCommon.h"
#include "TownManager.h"
#include "Ui/Dropdown.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Widget.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Game
{
    static loco_global<uint8_t[40], 0x00113DB20> _menuOptions;
    static loco_global<uint32_t, 0x009C86F8> _zoomTicks;
    static loco_global<uint8_t, 0x009C870C> _lastTownOption;
    static loco_global<uint8_t, 0x009C870D> _lastPortOption;
    static loco_global<uint8_t[18], 0x0050A006> _availableObjects;
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

    enum class LoadSaveDropdownId
    {
        loadGame,
        saveGame,
        about,
        options,
        screenshot,
        giantScreenshot,
        server,
        quitToMenu,
        quitToDesktop
    };

    static WindowEventList _events;

    static void onMouseDown(Window& window, WidgetIndex_t widgetIndex);
    static void onDropdown(Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    static void prepareDraw(Window& window);
    static void draw(Window& window, Gfx::RenderTarget* rt);

    // 0x00438B26
    void open()
    {
        _zoomTicks = 0;
        _lastTownOption = 0;
        _lastPortOption = 0;

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
        auto d = Dropdown::create()
                     .below(*window, widgetIndex)
                     .item(LoadSaveDropdownId::loadGame, StringIds::menu_load_game)
                     .item(LoadSaveDropdownId::saveGame, StringIds::menu_save_game)
                     .separator()
                     .item(LoadSaveDropdownId::about, StringIds::menu_about)
                     .item(LoadSaveDropdownId::options, StringIds::options)
                     .item(LoadSaveDropdownId::screenshot, StringIds::menu_screenshot)
                     .item(LoadSaveDropdownId::giantScreenshot, StringIds::menu_giant_screenshot);

        auto& newConfig = Config::get();
        if (newConfig.network.enabled)
        {
            d.separator();
            if (isNetworked())
            {
                if (isNetworkHost())
                {
                    d.item(LoadSaveDropdownId::server, StringIds::closeServer);
                }
                else
                {
                    d.item(LoadSaveDropdownId::server, StringIds::disconnect);
                }
            }
            else
            {
                d.item(LoadSaveDropdownId::server, StringIds::startServer);
            }
        }

        d.separator()
            .item(LoadSaveDropdownId::quitToMenu, StringIds::menu_quit_to_menu)
            .item(LoadSaveDropdownId::quitToDesktop, StringIds::menu_exit_openloco)
            .highlight(LoadSaveDropdownId::saveGame)
            .show();
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
        if (Config::get().hasFlags(Config::Flags::exportObjectsWithSaves))
            flags = S5::SaveFlags::packCustomObjects;

        if (!S5::exportGameStateToFile(path, flags))
            Error::open(StringIds::error_game_save_failed, StringIds::null);
    }

    static void startOrCloseServer()
    {
        if (isNetworked())
        {
            Network::close();
        }
        else
        {
            Network::openServer();
        }
    }

    // 0x0043B154
    static void loadsaveMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        auto id = Dropdown::getSelectedItem<LoadSaveDropdownId>(itemIndex);
        if (!id)
            return;

        switch (*id)
        {
            case LoadSaveDropdownId::loadGame:
                // Load game
                GameCommands::do_21(0, 0);
                break;

            case LoadSaveDropdownId::saveGame:
                // Save game
                prepareSaveGame();
                break;

            case LoadSaveDropdownId::about:
                About::open();
                break;

            case LoadSaveDropdownId::options:
                Options::open();
                break;

            case LoadSaveDropdownId::screenshot:
                Input::triggerScreenshotCountdown(10, Input::ScreenshotType::regular);
                break;

            case LoadSaveDropdownId::giantScreenshot:
                Input::triggerScreenshotCountdown(10, Input::ScreenshotType::giant);
                break;

            case LoadSaveDropdownId::server:
                startOrCloseServer();
                break;

            case LoadSaveDropdownId::quitToMenu:
                // Return to title screen
                GameCommands::do_21(0, 1);
                break;

            case LoadSaveDropdownId::quitToDesktop:
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

        if (Config::get().old.musicPlaying)
            Dropdown::setItemSelected(1);

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043B0B8
    static void audioMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
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
                auto& config = Config::get().old;
                if (config.musicPlaying)
                {
                    config.musicPlaying = false;
                    Audio::stopMusic();
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
        Dropdown::add(2, StringIds::open_scenario_options);
        Dropdown::add(3, StringIds::open_object_selection);
        Dropdown::add(4, 0);
        Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::cheat_enable_sandbox_mode);
        Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::cheat_allow_building_while_paused);
        Dropdown::add(7, StringIds::dropdown_without_checkmark, StringIds::cheat_allow_manual_driving);
        Dropdown::showBelow(window, widgetIndex, 8, 0);

        if (isSandboxMode())
            Dropdown::setItemSelected(5);

        if (isPauseOverrideEnabled())
            Dropdown::setItemSelected(6);

        if (isDriverCheatEnabled())
            Dropdown::setItemSelected(7);
    }

    static void cheatsMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
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

            case 2:
                ScenarioOptions::open();
                break;

            case 3:
                ObjectSelectionWindow::open();
                break;

            case 5:
                if (!isSandboxMode())
                    setScreenFlag(ScreenFlags::sandboxMode);
                else
                    clearScreenFlag(ScreenFlags::sandboxMode);
                break;

            case 6:
                if (!isPauseOverrideEnabled())
                    setScreenFlag(ScreenFlags::pauseOverrideEnabled);
                else
                    clearScreenFlag(ScreenFlags::pauseOverrideEnabled);
                break;

            case 7:
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

        assert(std::size(_availableObjects) >= std::size(availableTracks));
        // Legacy copy to available_objects remove when all users of 0x0050A006 accounted for
        std::copy(std::begin(availableTracks), std::end(availableTracks), std::begin(_availableObjects));
        _availableObjects[availableTracks.size()] = std::numeric_limits<uint8_t>::max();

        if (availableTracks.size() == 0)
            return;

        auto companyColour = CompanyManager::getPlayerCompanyColour();

        // Add available objects to Dropdown.
        uint16_t highlightedItem = 0;

        for (auto i = 0u; i < std::size(availableTracks); i++)
        {
            uint32_t objImage;
            string_id objStringId;

            auto objIndex = availableTracks[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = ObjectManager::get<RoadObject>(objIndex & 0x7F);
                objStringId = road->name;
                objImage = Gfx::recolour(road->image, companyColour);
            }
            else
            {
                auto track = ObjectManager::get<TrackObject>(objIndex);
                objStringId = track->name;
                objImage = Gfx::recolour(track->image, companyColour);
            }

            Dropdown::add(i, StringIds::menu_sprite_stringid_construction, { objImage, objStringId });

            if (objIndex == LastGameOptionManager::getLastRailRoad())
                highlightedItem = i;
        }

        Dropdown::showBelow(window, widgetIndex, std::size(availableTracks), 25, (1 << 6));
        Dropdown::setHighlightedItem(highlightedItem);
    }

    // 0x0043A39F
    static void railroadMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
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
            _menuOptions[ddIndex] = 0;
            ddIndex++;
        }

        if (addr<0x525FAD, int8_t>() != -1)
        {
            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_ship_port, StringIds::menu_ship_port });
            _menuOptions[ddIndex] = 1;
            ddIndex++;
        }

        if (ddIndex == 0)
            return;

        Dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));

        ddIndex = 0;
        if (_lastPortOption != _menuOptions[0])
            ddIndex++;

        Dropdown::setHighlightedItem(ddIndex);
    }

    // 0x0043AA0A
    static void portMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        _lastPortOption = _menuOptions[itemIndex];

        if (_lastPortOption == 0)
        {
            Construction::openWithFlags(1 << 31);
        }
        else if (_lastPortOption == 1)
        {
            Construction::openWithFlags(1 << 30);
        }
    }

    struct VehicleTypeInterfaceParam
    {
        uint32_t image;
        uint32_t buildImage;
        string_id buildString;
        string_id numSingular;
        string_id numPlural;
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

            uint32_t vehicle_image = Gfx::recolour(interface_param.buildImage, companyColour);

            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicle_image, interface_param.buildString });
            _menuOptions[ddIndex] = vehicleType;
            ddIndex++;
        }

        Dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));
        Dropdown::setHighlightedItem(LastGameOptionManager::getLastBuildVehiclesOption());
    }

    // 0x0043ADC7
    static void buildVehiclesMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        itemIndex = _menuOptions[itemIndex];
        LastGameOptionManager::setLastBuildVehiclesOption(itemIndex);

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

            if (v->has38Flags(Vehicles::Flags38::isGhost))
                continue;

            vehicle_counts[static_cast<uint8_t>(v->vehicleType)]++;
        }

        uint8_t ddIndex = 0;
        for (uint16_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((availableVehicles & (1 << vehicleType)) == 0)
                continue;

            auto& interfaceParam = VehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicleImage = Gfx::recolour(interfaceParam.image, companyColour);
            uint16_t vehicle_count = vehicle_counts[vehicleType];

            // TODO: replace with locale-based plurals.
            string_id vehicleStringId;
            if (vehicle_count == 1)
                vehicleStringId = interfaceParam.numSingular;
            else
                vehicleStringId = interfaceParam.numPlural;

            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicleImage, vehicleStringId, vehicle_count });
            _menuOptions[ddIndex] = vehicleType;
            ddIndex++;
        }

        Dropdown::showBelow(window, widgetIndex, ddIndex, 25, (1 << 6));
        Dropdown::setHighlightedItem(static_cast<uint8_t>(LastGameOptionManager::getLastVehicleType()));
    }

    // 0x0043ACEF
    static void vehiclesMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        auto vehicleType = VehicleType(_menuOptions[itemIndex]);
        LastGameOptionManager::setLastVehicleType(vehicleType);

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
    static void stationsMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex > 4)
            return;

        StationList::open(CompanyManager::getControllingId(), itemIndex);
    }

    // 0x0043A071
    static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuMouseDown(&window, widgetIndex);
                break;

            case Widx::cheats_menu:
                cheatsMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::railroad_menu:
                railroadMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::port_menu:
                portMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::build_vehicles_menu:
                buildVehiclesMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::vehicles_menu:
                vehiclesMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::stations_menu:
                stationsMenuMouseDown(&window, widgetIndex);
                break;

            default:
                Common::onMouseDown(&window, widgetIndex);
                break;
        }
    }

    static void onDropdown(Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Widx::cheats_menu:
                cheatsMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::railroad_menu:
                railroadMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::port_menu:
                portMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::build_vehicles_menu:
                buildVehiclesMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::vehicles_menu:
                vehiclesMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::stations_menu:
                stationsMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            default:
                Common::onDropdown(&window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x00439DE4
    static void draw(Window& window, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        Common::draw(window, rt);

        const auto companyColour = CompanyManager::getPlayerCompanyColour();

        if (window.widgets[Common::Widx::railroad_menu].type != WidgetType::none)
        {
            uint32_t x = window.widgets[Common::Widx::railroad_menu].left + window.x;
            uint32_t y = window.widgets[Common::Widx::railroad_menu].top + window.y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = LastGameOptionManager::getLastRailRoad();
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
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, window.getColour(WindowColour::tertiary).c());

            y--;
            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Common::Widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            drawingCtx.drawImage(rt, x, y, fg_image);

            y = window.widgets[Common::Widx::railroad_menu].top + window.y;
            drawingCtx.drawImage(rt, x, y, bg_image);
        }

        {
            uint32_t x = window.widgets[Common::Widx::vehicles_menu].left + window.x;
            uint32_t y = window.widgets[Common::Widx::vehicles_menu].top + window.y;

            static const uint32_t button_face_image_ids[] = {
                InterfaceSkin::ImageIds::vehicle_train_frame_0,
                InterfaceSkin::ImageIds::vehicle_buses_frame_0,
                InterfaceSkin::ImageIds::vehicle_trucks_frame_0,
                InterfaceSkin::ImageIds::vehicle_trams_frame_0,
                InterfaceSkin::ImageIds::vehicle_aircraft_frame_0,
                InterfaceSkin::ImageIds::vehicle_ships_frame_0,
            };

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t fg_image = Gfx::recolour(interface->img + button_face_image_ids[static_cast<uint8_t>(LastGameOptionManager::getLastVehicleType())], companyColour);
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, window.getColour(WindowColour::quaternary).c());

            y--;
            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Common::Widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            drawingCtx.drawImage(rt, x, y, fg_image);

            y = window.widgets[Common::Widx::vehicles_menu].top + window.y;
            drawingCtx.drawImage(rt, x, y, bg_image);
        }

        {
            uint32_t x = window.widgets[Common::Widx::build_vehicles_menu].left + window.x;
            uint32_t y = window.widgets[Common::Widx::build_vehicles_menu].top + window.y;

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
            uint32_t fg_image = Gfx::recolour(interface->img + build_vehicle_images[LastGameOptionManager::getLastBuildVehiclesOption()], companyColour);

            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Common::Widx::build_vehicles_menu))
                fg_image++;

            drawingCtx.drawImage(rt, x, y, fg_image);
        }
    }

    // 0x00439BCB
    static void prepareDraw(Window& window)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        if (!Audio::isAudioEnabled())
        {
            window.activatedWidgets |= (1 << Common::Widx::audio_menu);
            window.widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, window.getColour(WindowColour::primary).c());
        }
        else
        {
            window.activatedWidgets &= ~(1 << Common::Widx::audio_menu);
            window.widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, window.getColour(WindowColour::primary).c());
        }

        if (Config::get().cheatsMenuEnabled)
        {
            window.widgets[Widx::cheats_menu].type = WidgetType::toolbarTab;
            auto& baseWidget = window.widgets[Widx::cheats_menu];
            window.widgets[Common::Widx::zoom_menu].left = baseWidget.left + 14 + (baseWidget.width() * 1);
            window.widgets[Common::Widx::rotate_menu].left = baseWidget.left + 14 + (baseWidget.width() * 2);
            window.widgets[Common::Widx::view_menu].left = baseWidget.left + 14 + (baseWidget.width() * 3);
        }
        else
        {
            window.widgets[Widx::cheats_menu].type = WidgetType::none;
            auto& baseWidget = window.widgets[Common::Widx::audio_menu];
            window.widgets[Common::Widx::zoom_menu].left = baseWidget.left + 14 + (baseWidget.width() * 1);
            window.widgets[Common::Widx::rotate_menu].left = baseWidget.left + 14 + (baseWidget.width() * 2);
            window.widgets[Common::Widx::view_menu].left = baseWidget.left + 14 + (baseWidget.width() * 3);
        }

        if (_lastPortOption == 0
            && LastGameOptionManager::getLastAirport() != LastGameOptionManager::kNoLastOption
            && LastGameOptionManager::getLastShipPort() == LastGameOptionManager::kNoLastOption)
        {
            _lastPortOption = 1;
        }

        window.widgets[Common::Widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave);
        window.widgets[Widx::cheats_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_cogwheels);
        window.widgets[Common::Widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom);
        window.widgets[Common::Widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate);
        window.widgets[Common::Widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view);

        window.widgets[Common::Widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform);
        window.widgets[Common::Widx::railroad_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window.widgets[Common::Widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window.widgets[Common::Widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window.widgets[Common::Widx::build_vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);

        window.widgets[Common::Widx::vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        window.widgets[Common::Widx::stations_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_stations);

        if (_lastTownOption == 0)
            window.widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns);
        else
            window.widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries);

        if (_lastPortOption == 0)
            window.widgets[Common::Widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_airports);
        else
            window.widgets[Common::Widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_ports);

        if (LastGameOptionManager::getLastRoad() != LastGameOptionManager::kNoLastOption)
            window.widgets[Common::Widx::road_menu].type = WidgetType::toolbarTab;
        else
            window.widgets[Common::Widx::road_menu].type = WidgetType::none;

        if (LastGameOptionManager::getLastRailRoad() != LastGameOptionManager::kNoLastOption)
            window.widgets[Common::Widx::railroad_menu].type = WidgetType::toolbarTab;
        else
            window.widgets[Common::Widx::railroad_menu].type = WidgetType::none;

        if (LastGameOptionManager::getLastAirport() != LastGameOptionManager::kNoLastOption || LastGameOptionManager::getLastShipPort() != LastGameOptionManager::kNoLastOption)
            window.widgets[Common::Widx::port_menu].type = WidgetType::toolbarTab;
        else
            window.widgets[Common::Widx::port_menu].type = WidgetType::none;

        uint32_t x = std::max(640, Ui::width()) - 1;
        Common::rightAlignTabs(&window, x, { Common::Widx::towns_menu, Common::Widx::stations_menu, Common::Widx::vehicles_menu });
        x -= 11;
        Common::rightAlignTabs(&window, x, { Common::Widx::build_vehicles_menu });

        if (window.widgets[Common::Widx::port_menu].type != WidgetType::none)
        {
            Common::rightAlignTabs(&window, x, { Common::Widx::port_menu });
        }

        if (window.widgets[Common::Widx::road_menu].type != WidgetType::none)
        {
            Common::rightAlignTabs(&window, x, { Common::Widx::road_menu });
        }

        if (window.widgets[Common::Widx::railroad_menu].type != WidgetType::none)
        {
            Common::rightAlignTabs(&window, x, { Common::Widx::railroad_menu });
        }

        Common::rightAlignTabs(&window, x, { Common::Widx::terraform_menu });
    }
}
