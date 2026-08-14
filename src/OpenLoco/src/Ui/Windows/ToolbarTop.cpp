#include "Audio/Audio.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Jukebox.h"
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
#include "Scenario/ScenarioOptions.h"
#include "SceneManager.h"
#include "Tutorial.h"
#include "Ui/Dropdown.h"
#include "Ui/Screenshot.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ImageButtonAltWidget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

#include <OpenLoco/Utility/LookupTable.hpp>

namespace OpenLoco::Ui::Windows::ToolbarTop
{
    static uint8_t _defaultPortObjectId; // 0x009C870D
    static uint8_t _defaultTownObjectId; // 0x009C870C
    static uint32_t _zoomTicks;          // 0x009C86F8

    // Temporary storage for road menu dropdown (populated in mouseDown, consumed in dropdown callback)
    static AvailableTracksAndRoads _roadMenuObjects;

    // Temporary storage for railroad menu dropdown (populated in mouseDown, consumed in dropdown callback)
    static AvailableTracksAndRoads _railroadMenuObjects;

    enum widx
    {
        loadsave_menu,
        audio_menu,
        cheats_menu,
        map_generation_menu,

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

    namespace Widx
    {
        constexpr WidgetId kLoadsaveMenu{ "loadsave_menu" };
        constexpr WidgetId kAudioMenu{ "audio_menu" };
        constexpr WidgetId kCheatsMenu{ "cheats_menu" };
        constexpr WidgetId kMapGenerationMenu{ "map_generation_menu" };

        constexpr WidgetId kZoomMenu{ "zoom_menu" };
        constexpr WidgetId kRotateMenu{ "rotate_menu" };
        constexpr WidgetId kViewMenu{ "view_menu" };
        constexpr WidgetId kTerraformMenu{ "terraform_menu" };
        constexpr WidgetId kRailroadMenu{ "railroad_menu" };
        constexpr WidgetId kRoadMenu{ "road_menu" };
        constexpr WidgetId kPortMenu{ "port_menu" };
        constexpr WidgetId kBuildVehiclesMenu{ "build_vehicles_menu" };

        constexpr WidgetId kVehiclesMenu{ "vehicles_menu" };
        constexpr WidgetId kStationsMenu{ "stations_menu" };
        constexpr WidgetId kTownsMenu{ "towns_menu" };
    }

    static constexpr auto _widgets = makeWidgets(
        // Left-hand side
        Widgets::ImageButtonAlt(Widx::kLoadsaveMenu, { 0, 0 }, { 30, 28 }, WindowColour::primary),
        Widgets::ImageButtonAlt(Widx::kAudioMenu, { 30, 0 }, { 30, 28 }, WindowColour::primary),
        Widgets::ImageButtonAlt(Widx::kCheatsMenu, { 60, 0 }, { 30, 28 }, WindowColour::primary),
        Widgets::ImageButtonAlt(Widx::kMapGenerationMenu, { 60, 0 }, { 30, 28 }, WindowColour::primary),

        Widgets::ImageButtonAlt(Widx::kZoomMenu, { 104, 0 }, { 30, 28 }, WindowColour::secondary),
        Widgets::ImageButtonAlt(Widx::kRotateMenu, { 134, 0 }, { 30, 28 }, WindowColour::secondary),
        Widgets::ImageButtonAlt(Widx::kViewMenu, { 164, 0 }, { 30, 28 }, WindowColour::secondary),

        // Right-hand side
        Widgets::ImageButtonAlt(Widx::kTerraformMenu, { 267, 0 }, { 30, 28 }, WindowColour::tertiary),
        Widgets::ImageButtonAlt(Widx::kRailroadMenu, { 387, 0 }, { 30, 28 }, WindowColour::tertiary),
        Widgets::ImageButtonAlt(Widx::kRoadMenu, { 357, 0 }, { 30, 28 }, WindowColour::tertiary),
        Widgets::ImageButtonAlt(Widx::kPortMenu, { 417, 0 }, { 30, 28 }, WindowColour::tertiary),
        Widgets::ImageButtonAlt(Widx::kBuildVehiclesMenu, { 417, 0 }, { 30, 28 }, WindowColour::tertiary),

        Widgets::ImageButtonAlt(Widx::kVehiclesMenu, { 490, 0 }, { 30, 28 }, WindowColour::quaternary),
        Widgets::ImageButtonAlt(Widx::kStationsMenu, { 520, 0 }, { 30, 28 }, WindowColour::quaternary),
        Widgets::ImageButtonAlt(Widx::kTownsMenu, { 460, 0 }, { 30, 28 }, WindowColour::quaternary)

    );

    static const WindowEventList& getEvents();

    // 0x00438B26
    void open()
    {
        auto* window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            { Ui::width(), 28 },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        _defaultPortObjectId = 0;
        _defaultTownObjectId = 0;
        _zoomTicks = 0;

        auto* skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, skin->topToolbarPrimaryColour);
            window->setColour(WindowColour::secondary, skin->topToolbarSecondaryColour);
            window->setColour(WindowColour::tertiary, skin->topToolbarTertiaryColour);
            window->setColour(WindowColour::quaternary, skin->topToolbarQuaternaryColour);
        }
    }

    enum class LoadSaveDropdownId
    {
        loadGame,
        saveGame,
        loadLandscape,
        saveLandscape,
        about,
        options,
        screenshot,
        giantScreenshot,
        server,
        quitToMenu,
        quitToDesktop
    };

    // 0x0043B0F7
    static void loadsaveMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto d = Dropdown::create()
                     .below(self, widgetIndex);

        if (SceneManager::isEditorMode())
        {
            d.item(LoadSaveDropdownId::loadLandscape, StringIds::load_landscape);
            d.item(LoadSaveDropdownId::saveLandscape, StringIds::save_landscape);
        }
        else
        {
            d.item(LoadSaveDropdownId::loadGame, StringIds::menu_load_game);
            d.item(LoadSaveDropdownId::saveGame, StringIds::menu_save_game);
        }

        d.separator()
            .item(LoadSaveDropdownId::about, StringIds::menu_about)
            .item(LoadSaveDropdownId::options, StringIds::options)
            .item(LoadSaveDropdownId::screenshot, StringIds::menu_screenshot);

        // TODO: REMOVE WHEN REWORKING TUTORIALS (the if statement - keep the item) (and tutorial.h include above)
        if (OpenLoco::Tutorial::state() == OpenLoco::Tutorial::State::none)
        {
            d.item(LoadSaveDropdownId::giantScreenshot, StringIds::menu_giant_screenshot);
        }

        auto& newConfig = Config::get();
        if (newConfig.network.enabled)
        {
            d.separator();
            if (SceneManager::isNetworked())
            {
                if (SceneManager::isNetworkHost())
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
        ToolManager::toolCancel();

        if (SceneManager::isNetworked())
        {
            if (GameCommands::getUpdatingCompanyId() == CompanyManager::getControllingId())
            {
                GameCommands::do_72();
                MultiPlayer::setFlag(MultiPlayer::flags::flag_2);
            }
            return;
        }

        auto res = OpenLoco::Game::saveSaveGameOpen();
        if (!res)
        {
            // Cancelled by user
            Gfx::invalidateScreen();
            return;
        }

        auto path = fs::u8path(*res).replace_extension(S5::extensionSV5);
        OpenLoco::Game::setActiveSavePath(path.u8string());

        S5::SaveFlags flags = S5::SaveFlags::none;
        if (Config::get().exportObjectsWithSaves)
        {
            flags = S5::SaveFlags::packCustomObjects;
        }

        if (!S5::exportGameStateToFile(path, flags))
        {
            Error::open(StringIds::error_game_save_failed, StringIds::null);
        }
    }

    static void startOrCloseServer()
    {
        if (SceneManager::isNetworked())
        {
            Network::close();
        }
        else
        {
            Network::openServer();
        }
    }

    // 0x0043B154
    static void loadsaveMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        auto id = Dropdown::getSelectedItem<LoadSaveDropdownId>(itemIndex);
        if (!id)
        {
            return;
        }

        switch (*id)
        {
            case LoadSaveDropdownId::loadGame:
                // Load game
                {
                    GameCommands::LoadSaveQuitGameArgs loadGameArgs{};
                    loadGameArgs.loadQuitMode = LoadOrQuitMode::loadGamePrompt;
                    loadGameArgs.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                    GameCommands::doCommand(loadGameArgs, GameCommands::Flags::apply);
                }
                break;

            case LoadSaveDropdownId::saveGame:
                // Save game
                prepareSaveGame();
                break;

            case LoadSaveDropdownId::loadLandscape:
            {
                GameCommands::LoadSaveQuitGameArgs args{};
                args.loadQuitMode = LoadOrQuitMode::loadGamePrompt;
                args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
            }
            break;

            case LoadSaveDropdownId::saveLandscape:
            {
                if (Scenario::getOptions().editorStep == EditorController::Step::objectSelection)
                {
                    if (!ObjectSelectionWindow::tryCloseWindow())
                    {
                        // Try close has failed so do not open save window!
                        return;
                    }
                }
                WindowManager::closeAllFloatingWindows();
                ToolManager::toolCancel();

                // Save Landscape
                if (auto res = OpenLoco::Game::saveLandscapeOpen())
                {
                    OpenLoco::Game::saveLandscape(*res);
                    Gfx::invalidateScreen();
                }
                break;
            }

            case LoadSaveDropdownId::about:
                About::open();
                break;

            case LoadSaveDropdownId::options:
                Options::open();
                break;

            case LoadSaveDropdownId::screenshot:
                triggerScreenshotCountdown(10, ScreenshotType::regular);
                break;

            case LoadSaveDropdownId::giantScreenshot:
                triggerScreenshotCountdown(10, ScreenshotType::giant);
                break;

            case LoadSaveDropdownId::server:
                startOrCloseServer();
                break;

            case LoadSaveDropdownId::quitToMenu:
                // Return to title screen
                {
                    GameCommands::LoadSaveQuitGameArgs quitToMenuArgs{};
                    quitToMenuArgs.loadQuitMode = LoadOrQuitMode::returnToTitlePrompt;
                    quitToMenuArgs.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                    GameCommands::doCommand(quitToMenuArgs, GameCommands::Flags::apply);
                }
                break;

            case LoadSaveDropdownId::quitToDesktop:
                // Exit to desktop
                {
                    GameCommands::LoadSaveQuitGameArgs quitToDesktopArgs{};
                    quitToDesktopArgs.loadQuitMode = LoadOrQuitMode::quitGamePrompt;
                    quitToDesktopArgs.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                    GameCommands::doCommand(quitToDesktopArgs, GameCommands::Flags::apply);
                }
                break;
        }
    }

    // 0x0043B04B
    static void audioMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_mute);
        Dropdown::add(1, StringIds::dropdown_without_checkmark, StringIds::menu_play_music);
        Dropdown::add(2, 0);
        Dropdown::add(3, StringIds::menu_open_audio_options);
        Dropdown::add(4, StringIds::menu_open_jukebox);
        Dropdown::showBelow(&self, widgetIndex, 5, 0);

        if (!Audio::isAudioEnabled())
        {
            Dropdown::setItemSelected(0);
        }

        if (Config::get().audio.playJukeboxMusic)
        {
            Dropdown::setItemSelected(1);
        }

        if (SceneManager::isEditorMode())
        {
            Dropdown::setItemDisabled(1);
            Dropdown::setItemDisabled(4);
        }

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043B0B8
    static void audioMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        switch (itemIndex)
        {
            case 0: // "Mute"
                Audio::toggleSound();
                break;

            case 1: // "Play Music"
            {
                auto& config = Config::get().audio;
                if (config.playJukeboxMusic)
                {
                    Jukebox::disableMusic();
                }
                else
                {
                    Jukebox::enableMusic();
                }

                WindowManager::invalidate(WindowType::musicJukebox);
                break;
            }

            case 3:
                Options::openAudioSettings();
                break;

            case 4:
                MusicJukebox::open();
                break;
        }
    }

    static void cheatsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::cheats);
        Dropdown::add(1, StringIds::tile_inspector);
        Dropdown::add(2, StringIds::open_scenario_options);
        Dropdown::add(3, StringIds::open_object_selection);
        Dropdown::add(4, 0);
        Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::cheat_enable_sandbox_mode);
        Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::cheat_allow_building_while_paused);
        Dropdown::add(7, StringIds::dropdown_without_checkmark, StringIds::cheat_allow_manual_driving);
        Dropdown::showBelow(&self, widgetIndex, 8, 0);

        if (SceneManager::isSandboxMode())
        {
            Dropdown::setItemSelected(5);
        }

        if (SceneManager::isPauseOverrideEnabled())
        {
            Dropdown::setItemSelected(6);
        }

        if (SceneManager::isDriverCheatEnabled())
        {
            Dropdown::setItemSelected(7);
        }
    }

    static void cheatsMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = 0;
        }

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
                if (!SceneManager::isSandboxMode())
                {
                    SceneManager::addSceneFlags(SceneManager::Flags::sandboxMode);
                }
                else
                {
                    SceneManager::removeSceneFlags(SceneManager::Flags::sandboxMode);
                }
                break;

            case 6:
                if (!SceneManager::isPauseOverrideEnabled())
                {
                    SceneManager::addSceneFlags(SceneManager::Flags::pauseOverrideEnabled);
                }
                else
                {
                    SceneManager::removeSceneFlags(SceneManager::Flags::pauseOverrideEnabled);
                }
                break;

            case 7:
                if (!SceneManager::isDriverCheatEnabled())
                {
                    SceneManager::addSceneFlags(SceneManager::Flags::driverCheatEnabled);
                }
                else
                {
                    SceneManager::removeSceneFlags(SceneManager::Flags::driverCheatEnabled);
                }
                break;
        }
    }

    // 0x004402BC
    static void mapGenerationMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::landscape_generation_options);
        auto numItems = 1;

        if (Config::get().cheatsMenuEnabled)
        {
            Dropdown::add(1, StringIds::tile_inspector);
            numItems += 1;
        }

        Dropdown::showBelow(&self, widgetIndex, numItems, 0);
        Dropdown::setHighlightedItem(0);
    }

    // 0x004402DA
    static void mapGenerationMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        switch (itemIndex)
        {
            case 0:
                Windows::LandscapeGeneration::open();
                break;

            case 1:
                TileInspector::open();
                break;
        }
    }

    // 0x0043A78E
    static void zoomMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_zoom_in, StringIds::menu_zoom_in });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_zoom_out, StringIds::menu_zoom_out });

        static constexpr uint32_t kMapSpritesByRotation[] = {
            InterfaceSkin::ImageIds::toolbar_menu_map_north,
            InterfaceSkin::ImageIds::toolbar_menu_map_west,
            InterfaceSkin::ImageIds::toolbar_menu_map_south,
            InterfaceSkin::ImageIds::toolbar_menu_map_east,
        };

        uint32_t mapSprite = kMapSpritesByRotation[WindowManager::getCurrentRotation()];

        Dropdown::add(2, StringIds::menu_sprite_stringid, { interface->img + mapSprite, StringIds::menu_map });
        Dropdown::showBelow(&self, widgetIndex, 3, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow->viewports[0]->zoom == ZoomLevel::min)
        {
            Dropdown::setItemDisabled(0);
            Dropdown::setHighlightedItem(1);
        }

        if (mainWindow->viewports[0]->zoom == ZoomLevel::max)
        {
            Dropdown::setItemDisabled(1);
            _zoomTicks = 1000;
        }

        if (mainWindow->viewports[0]->zoom != ZoomLevel::max && _zoomTicks <= 32)
        {
            Dropdown::setHighlightedItem(1);
        }
    }

    // 0x0043A86D
    static void zoomMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        auto* mainWindow = WindowManager::getMainWindow();

        if (itemIndex == 0)
        {
            mainWindow->viewportZoomIn(false);
            TownManager::updateLabels();
            StationManager::updateLabels();
        }
        else if (itemIndex == 1)
        {
            _zoomTicks = 0;
            mainWindow->viewportZoomOut(false);
            TownManager::updateLabels();
            StationManager::updateLabels();
        }
        else if (itemIndex == 2)
        {
            MapWindow::open();
        }
    }

    // 0x0043A5C5
    static void rotateMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_rotate_clockwise, StringIds::menu_rotate_clockwise });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_rotate_anti_clockwise, StringIds::menu_rotate_anti_clockwise });
        Dropdown::showBelow(&self, widgetIndex, 2, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A624
    static void rotateMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        auto mouseButtonUsed = Input::getLastKnownButtonState();
        auto* mainWindow = WindowManager::getMainWindow();

        if (itemIndex == 1 || mouseButtonUsed == Input::MouseButton::rightPressed)
        {
            mainWindow->viewportRotateLeft();
            TownManager::updateLabels();
            StationManager::updateLabels();
            MapWindow::centerOnViewPoint();
        }
        else if (itemIndex == 0)
        {
            mainWindow->viewportRotateRight();
            TownManager::updateLabels();
            StationManager::updateLabels();
            MapWindow::centerOnViewPoint();
        }
    }

    // 0x0043ADF6
    static void viewMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_underground_view);
        Dropdown::add(1, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughTracks);
        Dropdown::add(2, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughRoads);
        Dropdown::add(3, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughTrees);
        Dropdown::add(4, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughBuildings);
        Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughScenery);
        Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughBridges);
        Dropdown::add(7, 0);
        Dropdown::add(8, StringIds::dropdown_without_checkmark, StringIds::menu_height_marks_on_land);
        Dropdown::add(9, StringIds::dropdown_without_checkmark, StringIds::menu_height_marks_on_tracks_roads);
        Dropdown::add(10, StringIds::dropdown_without_checkmark, StringIds::menu_one_way_direction_arrows);
        Dropdown::add(11, 0);
        Dropdown::add(12, StringIds::dropdown_without_checkmark, StringIds::menu_town_names_displayed);
        Dropdown::add(13, StringIds::dropdown_without_checkmark, StringIds::menu_station_names_displayed);
        Dropdown::showBelow(&self, widgetIndex, 14, 0);

        ViewportFlags current_viewport_flags = WindowManager::getMainWindow()->viewports[0]->flags;

        if ((current_viewport_flags & ViewportFlags::underground_view) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(0);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughTracks) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(1);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughRoads) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(2);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughTrees) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(3);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughBuildings) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(4);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughScenery) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(5);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughBridges) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(6);
        }

        if ((current_viewport_flags & ViewportFlags::height_marks_on_land) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(8);
        }

        if ((current_viewport_flags & ViewportFlags::height_marks_on_tracks_roads) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(9);
        }

        if ((current_viewport_flags & ViewportFlags::one_way_direction_arrows) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(10);
        }

        if ((current_viewport_flags & ViewportFlags::hideTownNames) == ViewportFlags::none)
        {
            Dropdown::setItemSelected(12);
        }

        if ((current_viewport_flags & ViewportFlags::hideStationNames) == ViewportFlags::none)
        {
            Dropdown::setItemSelected(13);
        }

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043AF37
    static void viewMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        auto* mainWindow = WindowManager::getMainWindow();
        auto* viewport = mainWindow->viewports[0];

        if (itemIndex == 0)
        {
            viewport->flags ^= ViewportFlags::underground_view;
        }
        else if (itemIndex == 1)
        {
            viewport->flags ^= ViewportFlags::seeThroughTracks;
        }
        else if (itemIndex == 2)
        {
            viewport->flags ^= ViewportFlags::seeThroughRoads;
        }
        else if (itemIndex == 3)
        {
            viewport->flags ^= ViewportFlags::seeThroughTrees;
        }
        else if (itemIndex == 4)
        {
            viewport->flags ^= ViewportFlags::seeThroughBuildings;
        }
        else if (itemIndex == 5)
        {
            viewport->flags ^= ViewportFlags::seeThroughScenery;
        }
        else if (itemIndex == 6)
        {
            viewport->flags ^= ViewportFlags::seeThroughBridges;
        }
        else if (itemIndex == 8)
        {
            viewport->flags ^= ViewportFlags::height_marks_on_land;
        }
        else if (itemIndex == 9)
        {
            viewport->flags ^= ViewportFlags::height_marks_on_tracks_roads;
        }
        else if (itemIndex == 10)
        {
            viewport->flags ^= ViewportFlags::one_way_direction_arrows;
        }
        else if (itemIndex == 12)
        {
            viewport->flags ^= ViewportFlags::hideTownNames;
        }
        else if (itemIndex == 13)
        {
            viewport->flags ^= ViewportFlags::hideStationNames;
        }

        mainWindow->invalidate();
    }

    // 0x0043A3C3
    static void terraformMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        auto land = ObjectManager::get<LandObject>(getGameState().defaultLandObjectId);
        auto water = ObjectManager::get<WaterObject>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_bulldozer, StringIds::menu_clear_area });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { land->mapPixelImage + Land::ImageIds::toolbar_terraform_land, StringIds::menu_adjust_land });
        Dropdown::add(2, StringIds::menu_sprite_stringid, { water->image + Water::ImageIds::kToolbarTerraformWater, StringIds::menu_adjust_water });
        Dropdown::add(3, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_plant_trees, StringIds::menu_plant_trees });
        Dropdown::add(4, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_build_walls, StringIds::menu_build_walls });
        Dropdown::showBelow(&self, widgetIndex, 5, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A4A8
    static void terraformMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        switch (itemIndex)
        {
            case 0:
                Terraform::openClearArea();
                break;

            case 1:
                Terraform::openAdjustLand();
                break;

            case 2:
                Terraform::openAdjustWater();
                break;

            case 3:
                Terraform::openPlantTrees();
                break;

            case 4:
                Terraform::openBuildWalls();
                break;
        }
    }

    // 0x0043A2B0
    static void railroadMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        // Load dropdown objects removing any that are not unlocked.
        // Note: This is not using player company id! This looks odd.
        _railroadMenuObjects = companyGetAvailableRailTracks(GameCommands::getUpdatingCompanyId());

        if (_railroadMenuObjects.size() == 0)
        {
            return;
        }

        auto companyColour = CompanyManager::getPlayerCompanyColour();

        // Add available objects to Dropdown.
        uint16_t highlightedItem = 0;

        for (auto i = 0u; i < _railroadMenuObjects.size(); i++)
        {
            uint32_t objImage;
            StringId objStringId;

            auto objIndex = _railroadMenuObjects[i];
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
                objImage = Gfx::recolour(track->image + TrackObj::ImageIds::kUiPreviewImage0, companyColour);
            }

            Dropdown::add(i, StringIds::menu_sprite_stringid_construction, { objImage, objStringId });

            if (objIndex == getGameState().defaultRailroadObjectId)
            {
                highlightedItem = i;
            }
        }

        Dropdown::showBelow(&self, widgetIndex, _railroadMenuObjects.size(), 25, (1 << 6));
        Dropdown::setHighlightedItem(highlightedItem);
    }

    // 0x0043A39F
    static void railroadMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == -1)
        {
            return;
        }

        uint8_t objIndex = _railroadMenuObjects[itemIndex];
        Construction::openWithFlags(objIndex);
    }

    // 0x0043A19F
    static void roadMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        _roadMenuObjects = companyGetAvailableRoads(CompanyManager::getControllingId());

        // Sanity check: any objects available?
        if (_roadMenuObjects.empty())
        {
            return;
        }

        auto companyColour = CompanyManager::getPlayerCompanyColour();

        // Add available objects to Dropdown.
        uint16_t highlightedItem = 0;
        for (auto i = 0U; i < _roadMenuObjects.size(); i++)
        {
            uint32_t objImage;
            StringId objStringId;

            auto objIndex = _roadMenuObjects[i];
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
                objImage = Gfx::recolour(track->image + TrackObj::ImageIds::kUiPreviewImage0, companyColour);
            }

            Dropdown::add(i, StringIds::menu_sprite_stringid_construction, { objImage, objStringId });

            if (objIndex == getGameState().defaultRoadObjectId)
            {
                highlightedItem = i;
            }
        }

        Dropdown::showBelow(&self, widgetIndex, _roadMenuObjects.size(), 25, (1 << 6));
        Dropdown::setHighlightedItem(highlightedItem);
    }

    // 0x0043A28C
    static void roadMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == -1)
        {
            return;
        }

        uint8_t objIndex = _roadMenuObjects[itemIndex];
        Construction::openWithFlags(objIndex);
    }

    // 0x0043A965
    static void portMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        uint8_t ddIndex = 0;
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        if (getGameState().lastAirport != 0xFF)
        {
            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_airport, StringIds::menu_airport });
            Dropdown::setMenuOption(ddIndex, 0);
            ddIndex++;
        }

        if (getGameState().lastShipPort != 0xFF)
        {
            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid_construction, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_ship_port, StringIds::menu_ship_port });
            Dropdown::setMenuOption(ddIndex, 1);
            ddIndex++;
        }

        if (ddIndex == 0)
        {
            return;
        }

        Dropdown::showBelow(&self, widgetIndex, ddIndex, 25, (1 << 6));

        ddIndex = 0;
        if (_defaultPortObjectId != Dropdown::getMenuOption(0))
        {
            ddIndex++;
        }

        Dropdown::setHighlightedItem(ddIndex);
    }

    // 0x0043AA0A
    static void portMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        _defaultPortObjectId = Dropdown::getMenuOption(itemIndex);

        if (_defaultPortObjectId == 0)
        {
            Construction::openWithFlags(1U << 31);
        }
        else if (_defaultPortObjectId == 1)
        {
            Construction::openWithFlags(1U << 30);
        }
    }

    struct VehicleTypeInterfaceParam
    {
        uint32_t image;
        uint32_t buildImage;
        StringId buildString;
        StringId numSingular;
        StringId numPlural;
    };

    // clang-format off
    static constexpr auto kVehicleTypeInterfaceParameters = Utility::buildLookupTable<VehicleType, VehicleTypeInterfaceParam>({
        { VehicleType::bus,      { InterfaceSkin::ImageIds::vehicle_buses_frame_0,      InterfaceSkin::ImageIds::build_vehicle_bus_frame_0,      StringIds::build_buses,    StringIds::num_buses_singular,     StringIds::num_buses_plural } },
        { VehicleType::aircraft, { InterfaceSkin::ImageIds::vehicle_aircraft_frame_0, InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_0, StringIds::build_aircraft, StringIds::num_aircrafts_singular, StringIds::num_aircrafts_plural } },
        { VehicleType::ship,     { InterfaceSkin::ImageIds::vehicle_ships_frame_0,     InterfaceSkin::ImageIds::build_vehicle_ship_frame_0,     StringIds::build_ships,    StringIds::num_ships_singular,     StringIds::num_ships_plural } },
        { VehicleType::train,    { InterfaceSkin::ImageIds::vehicle_train_frame_0,    InterfaceSkin::ImageIds::build_vehicle_train_frame_0,    StringIds::build_trains,   StringIds::num_trains_singular,    StringIds::num_trains_plural } },
        { VehicleType::tram,     { InterfaceSkin::ImageIds::vehicle_trams_frame_0,     InterfaceSkin::ImageIds::build_vehicle_tram_frame_0,     StringIds::build_trams,    StringIds::num_trams_singular,     StringIds::num_trams_plural } },
        { VehicleType::truck,    { InterfaceSkin::ImageIds::vehicle_trucks_frame_0,    InterfaceSkin::ImageIds::build_vehicle_truck_frame_0,    StringIds::build_trucks,   StringIds::num_trucks_singular,    StringIds::num_trucks_plural } },
    });
    // clang-format on

    // 0x0043AD1F
    static void buildVehiclesMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto company = CompanyManager::get(CompanyManager::getControllingId());
        uint16_t availableVehicles = company->availableVehicles;

        auto companyColour = CompanyManager::getPlayerCompanyColour();
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        uint8_t ddIndex = 0;
        for (uint8_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((availableVehicles & (1 << vehicleType)) == 0)
            {
                continue;
            }

            auto& interface_param = kVehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicle_image = Gfx::recolour(interface_param.buildImage, companyColour);

            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicle_image, interface_param.buildString });
            Dropdown::setMenuOption(ddIndex, vehicleType);

            ddIndex++;
        }

        Dropdown::showBelow(&self, widgetIndex, ddIndex, 25, (1 << 6));
        Dropdown::setHighlightedItem(enumValue(getGameState().defaultBuildVehicleType));
    }

    // 0x0043ADC7
    static void buildVehiclesMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == -1)
        {
            return;
        }

        itemIndex = Dropdown::getMenuOption(itemIndex);
        const auto vehicleType = static_cast<VehicleType>(itemIndex);
        getGameState().defaultBuildVehicleType = vehicleType;

        BuildVehicle::openByType(vehicleType);
    }

    // 0x0043ABCB
    static void vehiclesMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto player_company_id = CompanyManager::getControllingId();
        auto company = CompanyManager::get(player_company_id);
        uint16_t availableVehicles = company->availableVehicles;

        auto companyColour = CompanyManager::getPlayerCompanyColour();
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        uint16_t vehicle_counts[vehicleTypeCount]{ 0 };
        for (auto* v : VehicleManager::VehicleList())
        {
            if (v->owner != player_company_id)
            {
                continue;
            }

            if (v->has38Flags(Vehicles::Flags38::isGhost))
            {
                continue;
            }

            vehicle_counts[static_cast<uint8_t>(v->vehicleType)]++;
        }

        uint8_t ddIndex = 0;
        for (uint16_t vehicleType = 0; vehicleType < vehicleTypeCount; vehicleType++)
        {
            if ((availableVehicles & (1 << vehicleType)) == 0)
            {
                continue;
            }

            auto& interfaceParam = kVehicleTypeInterfaceParameters.at(static_cast<VehicleType>(vehicleType));

            uint32_t vehicleImage = Gfx::recolour(interfaceParam.image, companyColour);
            uint16_t vehicle_count = vehicle_counts[vehicleType];

            // TODO: replace with locale-based plurals.
            StringId vehicleStringId;
            if (vehicle_count == 1)
            {
                vehicleStringId = interfaceParam.numSingular;
            }
            else
            {
                vehicleStringId = interfaceParam.numPlural;
            }

            Dropdown::add(ddIndex, StringIds::menu_sprite_stringid, { interface->img + vehicleImage, vehicleStringId, vehicle_count });
            Dropdown::setMenuOption(ddIndex, vehicleType);

            ddIndex++;
        }

        Dropdown::showBelow(&self, widgetIndex, ddIndex, 25, (1 << 6));
        Dropdown::setHighlightedItem(static_cast<uint8_t>(getGameState().lastVehicleType));
    }

    // 0x0043ACEF
    static void vehiclesMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == -1)
        {
            return;
        }

        auto vehicleType = VehicleType(Dropdown::getMenuOption(itemIndex));
        getGameState().lastVehicleType = vehicleType;

        VehicleList::open(CompanyManager::getControllingId(), vehicleType);
    }

    // 0x0043A4E9
    static void stationsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
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
        Dropdown::showBelow(&self, widgetIndex, 5, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A596
    static void stationsMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex > 4)
        {
            return;
        }

        StationList::open(CompanyManager::getControllingId(), itemIndex);
    }

    // 0x0043A8CE
    static void townsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });
        Dropdown::showBelow(&self, widgetIndex, 2, 25, (1 << 6));
        Dropdown::setHighlightedItem(_defaultTownObjectId);
    }

    // 0x0043A932
    static void townsMenuDropdown([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == 0)
        {
            TownList::open();
            _defaultTownObjectId = 0;
        }
        else if (itemIndex == 1)
        {
            IndustryList::open();
            _defaultTownObjectId = 1;
        }
    }

    // 0x0043A071
    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, const WidgetId id)
    {
        switch (id)
        {
            case Widx::kLoadsaveMenu:
                loadsaveMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kAudioMenu:
                audioMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kCheatsMenu:
                cheatsMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kMapGenerationMenu:
                mapGenerationMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kZoomMenu:
                zoomMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kRotateMenu:
                rotateMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kViewMenu:
                viewMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kTerraformMenu:
                terraformMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kRoadMenu:
                roadMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kRailroadMenu:
                railroadMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kPortMenu:
                portMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kBuildVehiclesMenu:
                buildVehiclesMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kVehiclesMenu:
                vehiclesMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kStationsMenu:
                stationsMenuMouseDown(self, widgetIndex);
                break;

            case Widx::kTownsMenu:
                townsMenuMouseDown(self, widgetIndex);
                break;
        }
    }

    static void onMouseHover(Window& self, WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (Config::get().toolbarAutoMenu)
        {
            onMouseDown(self, widgetIndex, id);
        }
    }

    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, const WidgetId id, int16_t itemIndex)
    {
        switch (id)
        {
            case Widx::kLoadsaveMenu:
                loadsaveMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kAudioMenu:
                audioMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kCheatsMenu:
                cheatsMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kMapGenerationMenu:
                mapGenerationMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kZoomMenu:
                zoomMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kRotateMenu:
                rotateMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kViewMenu:
                viewMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kTerraformMenu:
                terraformMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kRailroadMenu:
                railroadMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kRoadMenu:
                roadMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kPortMenu:
                portMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kBuildVehiclesMenu:
                buildVehiclesMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kVehiclesMenu:
                vehiclesMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kStationsMenu:
                stationsMenuDropdown(self, widgetIndex, itemIndex);
                break;

            case Widx::kTownsMenu:
                townsMenuDropdown(self, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x00439DE4
    static void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        const auto companyColour = CompanyManager::getPlayerCompanyColour();

        if (!self.widgets[widx::railroad_menu].hidden)
        {
            uint32_t x = self.widgets[widx::railroad_menu].left;
            uint32_t y = self.widgets[widx::railroad_menu].top;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = getGameState().defaultRailroadObjectId;
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = ObjectManager::get<RoadObject>(ebx);
                fg_image = Gfx::recolour(obj->image, companyColour);
            }
            else
            {
                auto obj = ObjectManager::get<TrackObject>(ebx);
                fg_image = Gfx::recolour(obj->image + TrackObj::ImageIds::kUiPreviewImage0, companyColour);
            }

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, self.getColour(WindowColour::tertiary).c());

            y--;
            if (Input::isDropdownActive(Ui::WindowType::topToolbar, self.number, widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            drawingCtx.drawImage(ZoomLevel::full, x, y, fg_image);

            y = self.widgets[widx::railroad_menu].top;
            drawingCtx.drawImage(ZoomLevel::full, x, y, bg_image);
        }

        if (!self.widgets[widx::vehicles_menu].hidden)
        {
            uint32_t x = self.widgets[widx::vehicles_menu].left;
            uint32_t y = self.widgets[widx::vehicles_menu].top;

            static constexpr uint32_t button_face_image_ids[] = {
                InterfaceSkin::ImageIds::vehicle_train_frame_0,
                InterfaceSkin::ImageIds::vehicle_buses_frame_0,
                InterfaceSkin::ImageIds::vehicle_trucks_frame_0,
                InterfaceSkin::ImageIds::vehicle_trams_frame_0,
                InterfaceSkin::ImageIds::vehicle_aircraft_frame_0,
                InterfaceSkin::ImageIds::vehicle_ships_frame_0,
            };

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t fg_image = Gfx::recolour(interface->img + button_face_image_ids[static_cast<uint8_t>(getGameState().lastVehicleType)], companyColour);
            uint32_t bg_image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, self.getColour(WindowColour::quaternary).c());

            y--;
            if (Input::isDropdownActive(Ui::WindowType::topToolbar, self.number, widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            drawingCtx.drawImage(ZoomLevel::full, x, y, fg_image);

            y = self.widgets[widx::vehicles_menu].top;
            drawingCtx.drawImage(ZoomLevel::full, x, y, bg_image);
        }

        if (!self.widgets[widx::build_vehicles_menu].hidden)
        {
            uint32_t x = self.widgets[widx::build_vehicles_menu].left;
            uint32_t y = self.widgets[widx::build_vehicles_menu].top;

            static constexpr uint32_t kBuildVehicleImages[] = {
                InterfaceSkin::ImageIds::toolbar_build_vehicle_train,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_bus,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_truck,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_tram,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_airplane,
                InterfaceSkin::ImageIds::toolbar_build_vehicle_boat,
            };

            // Figure out what icon to show on the button face.
            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t fg_image = Gfx::recolour(interface->img + kBuildVehicleImages[enumValue(getGameState().defaultBuildVehicleType)], companyColour);

            if (Input::isDropdownActive(Ui::WindowType::topToolbar, self.number, widx::build_vehicles_menu))
            {
                fg_image++;
            }

            drawingCtx.drawImage(ZoomLevel::full, x, y, fg_image);
        }
    }

    // 0x00439DE4
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        self.draw(drawingCtx);

        drawTabs(self, drawingCtx);

        const auto companyColour = CompanyManager::getPlayerCompanyColour();
        const auto defaultRoadObjectId = getGameState().defaultRoadObjectId;

        if (!self.widgets[widx::road_menu].hidden && defaultRoadObjectId != 0xFF)
        {
            uint32_t x = self.widgets[widx::road_menu].left;
            uint32_t y = self.widgets[widx::road_menu].top;
            uint32_t fgImage = 0;

            // Figure out what icon to show on the button face.
            bool isRoad = defaultRoadObjectId & (1 << 7);
            if (isRoad)
            {
                auto obj = ObjectManager::get<RoadObject>(defaultRoadObjectId & ~(1 << 7));
                fgImage = Gfx::recolour(obj->image, companyColour);
            }
            else
            {
                auto obj = ObjectManager::get<TrackObject>(defaultRoadObjectId);
                fgImage = Gfx::recolour(obj->image + TrackObj::ImageIds::kUiPreviewImage0, companyColour);
            }

            y--;
            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t bgImage = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, self.getColour(WindowColour::tertiary).c());

            if (Input::isDropdownActive(Ui::WindowType::topToolbar, self.number, widx::road_menu))
            {
                y++;
                bgImage++;
            }

            drawingCtx.drawImage(ZoomLevel::full, x, y, fgImage);

            y = self.widgets[widx::road_menu].top;
            drawingCtx.drawImage(ZoomLevel::full, x, y, bgImage);
        }
    }

    static void onUpdate([[maybe_unused]] Window& self)
    {
        _zoomTicks++;
    }

    // 0x0043A17E
    static void onResize(Window& self)
    {
        auto main = WindowManager::getMainWindow();
        if (main == nullptr)
        {
            self.setDisabledWidgetsAndInvalidate(widx::zoom_menu | widx::rotate_menu);
        }
        else
        {
            self.setDisabledWidgetsAndInvalidate(0);
        }
    }

    [[nodiscard]] static uint32_t leftAlignButtons(Window& self, uint32_t x, const std::initializer_list<uint32_t> widxs)
    {
        for (const auto& widx : widxs)
        {
            auto& widget = self.widgets[widx];
            if (widget.hidden)
            {
                continue;
            }

            widget.left = x;
            widget.right = x + 29;
            x += 30;
        }
        return x;
    }

    [[nodiscard]] static uint32_t rightAlignButtons(Window& self, uint32_t x, const std::initializer_list<uint32_t> widxs)
    {
        for (const auto& widx : widxs)
        {
            auto& widget = self.widgets[widx];
            if (widget.hidden)
            {
                continue;
            }

            widget.right = x;
            widget.left = x - 29;
            x -= 30;
        }
        return x;
    }

    static void centreToolbar(Window& self)
    {
        auto numVisibleWidgets = 0;
        for (auto& widget : self.widgets)
        {
            if (!widget.hidden)
            {
                numVisibleWidgets++;
            }
        }

        auto totalWidth = numVisibleWidgets * 30 + (4 * 11);

        // Left-hand side
        uint32_t x = std::max(0, (Ui::width() - totalWidth) / 2);
        x = leftAlignButtons(self, x, { widx::loadsave_menu, widx::audio_menu, widx::cheats_menu, widx::map_generation_menu });
        x += 11;
        x = leftAlignButtons(self, x, { widx::zoom_menu, widx::rotate_menu, widx::view_menu });

        // Right-hand side
        x += 11;
        x = leftAlignButtons(self, x, { widx::terraform_menu, widx::railroad_menu, widx::road_menu, widx::port_menu, widx::build_vehicles_menu });
        x += 11;
        x = leftAlignButtons(self, x, { widx::vehicles_menu, widx::stations_menu, widx::towns_menu });
    }

    static void justifyToolbar(Window& self)
    {
        // Left-hand side
        uint32_t x = 0;
        x = leftAlignButtons(self, x, { widx::loadsave_menu, widx::audio_menu, widx::cheats_menu, widx::map_generation_menu });
        x += 11;
        x = leftAlignButtons(self, x, { widx::zoom_menu, widx::rotate_menu, widx::view_menu });

        // Right-hand side
        x = std::max(640, Ui::width()) - 1;
        x = rightAlignButtons(self, x, { widx::towns_menu, widx::stations_menu, widx::vehicles_menu });
        x -= 11;
        x = rightAlignButtons(self, x, { widx::build_vehicles_menu, widx::port_menu, widx::road_menu, widx::railroad_menu, widx::terraform_menu });
    }

    // 0x00439BCB
    static void prepareDraw(Window& self)
    {
        // Hide buttons while in editor
        const bool isEditor = SceneManager::isEditorMode();
        const bool isLandscapeEditor = EditorController::getCurrentStep() == EditorController::Step::landscapeEditor;

        // Left-hand side
        self.widgets[widx::cheats_menu].hidden = isEditor || !Config::get().cheatsMenuEnabled;
        self.widgets[widx::map_generation_menu].hidden = !isEditor || !isLandscapeEditor;
        self.widgets[widx::zoom_menu].hidden = isEditor && !isLandscapeEditor;
        self.widgets[widx::rotate_menu].hidden = isEditor && !isLandscapeEditor;
        self.widgets[widx::view_menu].hidden = isEditor && !isLandscapeEditor;

        // Right-hand side
        self.widgets[widx::terraform_menu].hidden = isEditor && !isLandscapeEditor;
        self.widgets[widx::railroad_menu].hidden = isEditor || getGameState().defaultRailroadObjectId == 0xFF;
        self.widgets[widx::road_menu].hidden = (isEditor && !isLandscapeEditor) || getGameState().defaultRoadObjectId == 0xFF;
        self.widgets[widx::port_menu].hidden = isEditor || (getGameState().lastAirport == 0xFF && getGameState().lastShipPort == 0xFF);
        self.widgets[widx::build_vehicles_menu].hidden = isEditor;

        self.widgets[widx::vehicles_menu].hidden = isEditor;
        self.widgets[widx::stations_menu].hidden = isEditor;
        self.widgets[widx::towns_menu].hidden = isEditor && !isLandscapeEditor;

        const auto* interface = ObjectManager::get<InterfaceSkinObject>();
        if (!Audio::isAudioEnabled())
        {
            self.activatedWidgets |= (1 << widx::audio_menu);
            self.widgets[widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, self.getColour(WindowColour::primary).c());
        }
        else
        {
            self.activatedWidgets &= ~(1 << widx::audio_menu);
            self.widgets[widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, self.getColour(WindowColour::primary).c());
        }

        const bool cheatsOn = Config::get().cheatsMenuEnabled;
        const auto& refWidget = self.widgets[cheatsOn ? enumValue(widx::cheats_menu) : enumValue(widx::audio_menu)];
        const auto offsetWidget = [&self, refWidget](uint8_t widgetIndex, uint8_t index) {
            auto& widget = self.widgets[widgetIndex];
            widget.left = refWidget.left + 14 + (refWidget.width() * index);
            widget.right = widget.left + refWidget.width() - 1;
        };

        offsetWidget(widx::zoom_menu, 1);
        offsetWidget(widx::rotate_menu, 2);
        offsetWidget(widx::view_menu, 3);

        if (_defaultPortObjectId == 0
            && getGameState().lastAirport == 0xFF
            && getGameState().lastShipPort != 0xFF)
        {
            _defaultPortObjectId = 1;
        }

        self.widgets[widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave);
        self.widgets[widx::cheats_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_cogwheels);
        self.widgets[widx::map_generation_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_cogwheels);
        self.widgets[widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom);
        self.widgets[widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate);
        self.widgets[widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view);

        self.widgets[widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform);
        self.widgets[widx::railroad_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        self.widgets[widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        self.widgets[widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        self.widgets[widx::build_vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);

        self.widgets[widx::vehicles_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);
        self.widgets[widx::stations_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_stations);

        if (_defaultTownObjectId == 0)
        {
            self.widgets[widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns);
        }
        else
        {
            self.widgets[widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries);
        }

        if (_defaultPortObjectId == 0)
        {
            self.widgets[widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_airports);
        }
        else
        {
            self.widgets[widx::port_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_ports);
        }

        if (Config::get().toolbarButtonsCentred)
        {
            centreToolbar(self);
        }
        else
        {
            justifyToolbar(self);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onResize = onResize,
        .onMouseHover = onMouseHover,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
