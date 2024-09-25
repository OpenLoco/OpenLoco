#include "Shortcuts.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/SetGameSpeed.h"
#include "GameCommands/General/TogglePause.h"
#include "Input.h"
#include "LastGameOptionManager.h"
#include "Localisation/StringIds.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "Ui/Screenshot.h"
#include "Ui/TextInput.h"
#include "Ui/ToolManager.h"
#include "Ui/WindowManager.h"
#include "Windows/Construction/Construction.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <Vehicles/VehicleManager.h>
#include <array>
#include <unordered_map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::Input::Shortcuts
{
    // 0x004BF089
    static void closeTopmostWindow()
    {
        WindowManager::closeTopmost();
    }

    // 0x004BF0B6
    static void closeAllFloatingWindows()
    {
        WindowManager::closeAllFloatingWindows();
    }

    // 0x004BF0BC
    static void cancelConstructionMode()
    {
        auto* w = WindowManager::find(WindowType::error);
        if (w != nullptr)
        {
            WindowManager::close(w);
            return;
        }

        if (!Ui::Windows::Vehicle::cancelVehicleTools())
        {
            ToolManager::toolCancel();
        }
    }

    // 0x004BF0E6
    static void pauseUnpauseGame()
    {
        if (isEditorMode())
            return;

        GameCommands::doCommand(GameCommands::PauseGameArgs{}, GameCommands::Flags::apply);
    }

    // 0x004BF0FE
    static void zoomViewOut()
    {
        Window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportZoomOut(false);
        TownManager::updateLabels();
        StationManager::updateLabels();
    }

    // 0x004BF115
    static void zoomViewIn()
    {
        Window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportZoomIn(false);
        TownManager::updateLabels();
        StationManager::updateLabels();
    }

    // 0x004BF12C
    static void rotateView()
    {
        Window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportRotateRight();
        TownManager::updateLabels();
        StationManager::updateLabels();
        Windows::MapWindow::centerOnViewPoint();
    }

    // 0x004BF148
    static void rotateConstructionObject()
    {
        if (Windows::Vehicle::rotate())
            return;

        auto window = WindowManager::find(WindowType::terraform);
        if (window != nullptr)
        {
            if (Ui::Windows::Terraform::rotate(*window))
                return;
        }

        // 0x004A5D48
        window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
        {
            if (Ui::Windows::Construction::rotate(*window))
                return;
        }

        window = WindowManager::find(WindowType::townList);
        if (window != nullptr)
        {
            if (Ui::Windows::TownList::rotate(*window))
                return;
        }

        window = WindowManager::find(WindowType::company);
        if (window != nullptr)
        {
            if (Ui::Windows::CompanyWindow::rotate(*window))
                return;
        }
    }

    // 0x004BF18A
    static void toggleUndergroundView()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::underground_view;
        window->invalidate();
    }

    static void toggleSeeThroughBuildings()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::seeThroughBuildings;
        window->invalidate();
    }

    static void toggleSeeThroughBridges()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::seeThroughBridges;
        window->invalidate();
    }

    static void toggleSeeThroughRoads()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::seeThroughRoads;
        window->invalidate();
    }

    // 0x004BF19E
    static void toggleSeeThroughScenery()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::seeThroughScenery;
        window->invalidate();
    }

    // 0x004BF194
    static void toggleSeeThroughTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::seeThroughTracks;
        window->invalidate();
    }

    static void toggleSeeThroughTrees()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::seeThroughTrees;
        window->invalidate();
    }

    // 0x004BF1A8
    static void toggleHeightMarksOnLand()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::height_marks_on_land;
        window->invalidate();
    }

    // 0x004BF1B2
    static void toggleHeightMarksOnTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::height_marks_on_tracks_roads;
        window->invalidate();
    }

    // 0x004BF1BC
    static void toggleDirArrowsOnTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::one_way_direction_arrows;
        window->invalidate();
    }

    // 0x004BF1C6
    static void adjustLand()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::Terraform::openAdjustLand();
    }

    // 0x004BF1E1
    static void adjustWater()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::Terraform::openAdjustWater();
    }

    // 0x004BF1FC
    static void plantTrees()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::Terraform::openPlantTrees();
    }

    // 0x004BF217
    static void bulldozeArea()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::Terraform::openClearArea();
    }

    // 0x004BF232
    static void buildTracks()
    {
        if (isEditorMode())
            return;

        if (LastGameOptionManager::getLastRailRoad() == LastGameOptionManager::kNoLastOption)
            return;

        Windows::Construction::openWithFlags(LastGameOptionManager::getLastRailRoad());
    }

    // 0x004BF24F
    static void buildRoads()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        if (LastGameOptionManager::getLastRoad() == LastGameOptionManager::kNoLastOption)
            return;

        Windows::Construction::openWithFlags(LastGameOptionManager::getLastRoad());
    }

    // 0x004BF276
    static void buildAirports()
    {
        if (isEditorMode())
            return;

        if (LastGameOptionManager::getLastAirport() == LastGameOptionManager::kNoLastOption)
            return;

        Windows::Construction::openWithFlags(1U << 31);
    }

    // 0x004BF295
    static void buildShipPorts()
    {
        if (isEditorMode())
            return;

        if (LastGameOptionManager::getLastShipPort() == LastGameOptionManager::kNoLastOption)
            return;

        Windows::Construction::openWithFlags(1U << 30);
    }

    // 0x004BF2B4
    static void buildNewVehicles()
    {
        if (isEditorMode())
            return;

        if (LastGameOptionManager::getLastBuildVehiclesOption() == LastGameOptionManager::kNoLastOption)
            return;

        Windows::BuildVehicle::open(LastGameOptionManager::getLastBuildVehiclesOption(), 1U << 31);
    }

    // 0x004BF2D1
    static void showVehiclesList()
    {
        if (isEditorMode())
            return;

        Windows::VehicleList::open(CompanyManager::getControllingId(), LastGameOptionManager::getLastVehicleType());
    }

    // 0x004BF2F0
    static void showStationsList()
    {
        if (isEditorMode())
            return;

        Windows::StationList::open(CompanyManager::getControllingId(), 0);
    }

    // 0x004BF308
    static void showTownsList()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::TownList::open();
    }

    // 0x004BF323
    static void showIndustriesList()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::IndustryList::open();
    }

    // 0x004BF33E
    static void showMap()
    {
        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
            return;

        Windows::MapWindow::open();
    }

    // 0x004BF359
    static void showCompaniesList()
    {
        if (isEditorMode())
            return;

        Ui::Windows::CompanyList::open();
    }

    // 0x004BF36A
    static void showCompanyInformation()
    {
        if (isEditorMode())
            return;

        Ui::Windows::CompanyWindow::open(CompanyManager::getControllingId());
    }

    // 0x004BF382
    static void showFinances()
    {
        if (isEditorMode())
            return;

        Windows::CompanyWindow::openFinances(CompanyManager::getControllingId());
    }

    // 0x004BF39A
    static void showAnnouncementsList()
    {
        if (isEditorMode())
            return;

        Windows::MessageWindow::open();
    }

    static void showOptionsWindow()
    {
        Windows::Options::open();
    }

    // 0x004BF3AB
    static void makeScreenshot()
    {
        Ui::triggerScreenshotCountdown(2, Ui::ScreenshotType::regular);
    }

    // 0x004BF3B3
    static void toggleLastAnnouncement()
    {
        if (isEditorMode())
            return;

        auto window = WindowManager::find(WindowType::news);
        if (window)
        {
            Windows::NewsWindow::close(window);
        }
        else
        {
            Windows::NewsWindow::openLastMessage();
        }
    }

    // 0x004BF3DC
    static void sendMessage()
    {
        if (isEditorMode())
            return;

        if (!isNetworked())
            return;

        if (isTitleMode())
        {
            auto* caller = WindowManager::find(WindowType::titleMenu);
            if (caller == nullptr)
                return;

            Windows::TitleMenu::beginSendChatMessage(*caller);
        }
        else
        {
            auto* caller = WindowManager::find(WindowType::timeToolbar);
            if (caller == nullptr)
                return;

            Windows::TimePanel::beginSendChatMessage(*caller);
        }
    }

    static void constructionPreviousTab()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Common::previousTab(window);
    }

    static void constructionNextTab()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Common::nextTab(window);
    }

    static void constructionPreviousTrackPiece()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::previousTrackPiece(window);
    }

    static void constructionNextTrackPiece()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::nextTrackPiece(window);
    }

    static void constructionPreviousSlope()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::previousSlope(window);
    }

    static void constructionNextSlope()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::nextSlope(window);
    }

    static void constructionBuildAtCurrentPos()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::buildAtCurrentPos(window);
    }

    static void constructionRemoveAtCurrentPos()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::removeAtCurrentPos(window);
    }

    static void constructionSelectPosition()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
            Ui::Windows::Construction::Construction::selectPosition(window);
    }

    static void gameSpeedNormal()
    {
        GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::Normal }, GameCommands::Flags::apply);
    }

    static void gameSpeedFastForward()
    {
        GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::FastForward }, GameCommands::Flags::apply);
    }

    static void gameSpeedExtraFastForward()
    {
        GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::ExtraFastForward }, GameCommands::Flags::apply);
    }

    static void honkAllHorns()
    {
        // GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::MAX }, GameCommands::Flags::apply);
        VehicleManager::honkAllTrains();
    }

    void initialize()
    {
        // clang-format off
        ShortcutManager::add(Shortcut::closeTopmostWindow,              StringIds::shortcut_close_topmost_window,               closeTopmostWindow,             "closeTopmostWindow",               "Backspace");
        ShortcutManager::add(Shortcut::closeAllFloatingWindows,         StringIds::shortcut_close_all_floating_windows,         closeAllFloatingWindows,        "closeAllFloatingWindows",          "Left Shift+Backspace");
        ShortcutManager::add(Shortcut::cancelConstructionMode,          StringIds::shortcut_cancel_construction_mode,           cancelConstructionMode,         "cancelConstructionMode",           "Escape");
        ShortcutManager::add(Shortcut::pauseUnpauseGame,                StringIds::shortcut_pause_unpause_game,                 pauseUnpauseGame,               "pauseUnpauseGame",                 "Pause");
        ShortcutManager::add(Shortcut::showOptionsWindow,               StringIds::shortcut_show_options_window,                showOptionsWindow,              "showOptionsWindow",                "");
        ShortcutManager::add(Shortcut::zoomViewOut,                     StringIds::shortcut_zoom_view_out,                      zoomViewOut,                    "zoomViewOut",                      "PageUp");
        ShortcutManager::add(Shortcut::zoomViewIn,                      StringIds::shortcut_zoom_view_in,                       zoomViewIn,                     "zoomViewIn",                       "PageDown");
        ShortcutManager::add(Shortcut::rotateView,                      StringIds::shortcut_rotate_view,                        rotateView,                     "rotateView",                       "Return");
        ShortcutManager::add(Shortcut::rotateConstructionObject,        StringIds::shortcut_rotate_construction_object,         rotateConstructionObject,       "rotateConstructionObject",         "Z");
        ShortcutManager::add(Shortcut::toggleUndergroundView,           StringIds::shortcut_toggle_underground_view,            toggleUndergroundView,          "toggleUndergroundView",            "1");
        ShortcutManager::add(Shortcut::toggleSeeThroughTracks,          StringIds::shortcutSeeThroughTracks,                    toggleSeeThroughTracks,         "toggleSeeThroughTracks",           "2");
        ShortcutManager::add(Shortcut::toggleSeeThroughRoads,           StringIds::shortcutSeeThroughRoads,                     toggleSeeThroughRoads,          "toggleSeeThroughRoads",            "3");
        ShortcutManager::add(Shortcut::toggleSeeThroughTrees,           StringIds::shortcutSeeThroughTrees,                     toggleSeeThroughTrees,          "toggleSeeThroughTrees",            "4");
        ShortcutManager::add(Shortcut::toggleSeeThroughBuildings,       StringIds::shortcutSeeThroughBuildings,                 toggleSeeThroughBuildings,      "toggleSeeThroughBuildings",        "5");
        ShortcutManager::add(Shortcut::toggleSeeThroughBridges,         StringIds::shortcutSeeThroughBridges,                   toggleSeeThroughBridges,        "toggleSeeThroughBridges",          "6");
        ShortcutManager::add(Shortcut::toggleSeeThroughScenery,         StringIds::shortcutSeeThroughScenery,                   toggleSeeThroughScenery,        "toggleSeeThroughScenery",          "7");
        ShortcutManager::add(Shortcut::toggleHeightMarksOnLand,         StringIds::shortcut_toggle_height_marks_on_land,        toggleHeightMarksOnLand,        "toggleHeightMarksOnLand",          "8");
        ShortcutManager::add(Shortcut::toggleHeightMarksOnTracks,       StringIds::shortcut_toggle_height_marks_on_tracks,      toggleHeightMarksOnTracks,      "toggleHeightMarksOnTracks",        "9");
        ShortcutManager::add(Shortcut::toggleDirArrowsonTracks,         StringIds::shortcut_toggle_dir_arrows_on_tracks,        toggleDirArrowsOnTracks,        "toggleDirArrowsOnTracks",          "0");
        ShortcutManager::add(Shortcut::adjustLand,                      StringIds::shortcut_adjust_land,                        adjustLand,                     "adjustLand",                       "L");
        ShortcutManager::add(Shortcut::adjustWater,                     StringIds::shortcut_adjust_water,                       adjustWater,                    "adjustWater",                      "W");
        ShortcutManager::add(Shortcut::plantTrees,                      StringIds::shortcut_plant_trees,                        plantTrees,                     "plantTrees",                       "P");
        ShortcutManager::add(Shortcut::bulldozeArea,                    StringIds::shortcut_bulldoze_area,                      bulldozeArea,                   "bulldozeArea",                     "X");
        ShortcutManager::add(Shortcut::buildTracks,                     StringIds::shortcut_build_tracks,                       buildTracks,                    "buildTracks",                      "T");
        ShortcutManager::add(Shortcut::buildRoads,                      StringIds::shortcut_build_roads,                        buildRoads,                     "buildRoads",                       "R");
        ShortcutManager::add(Shortcut::buildAirports,                   StringIds::shortcut_build_airports,                     buildAirports,                  "buildAirports",                    "A");
        ShortcutManager::add(Shortcut::buildShipPorts,                  StringIds::shortcut_build_ship_ports,                   buildShipPorts,                 "buildShipPorts",                   "D");
        ShortcutManager::add(Shortcut::buildNewVehicles,                StringIds::shortcut_build_new_vehicles,                 buildNewVehicles,               "buildNewVehicles",                 "N");
        ShortcutManager::add(Shortcut::showVehiclesList,                StringIds::shortcut_show_vehicles_list,                 showVehiclesList,               "showVehiclesList",                 "V");
        ShortcutManager::add(Shortcut::showStationsList,                StringIds::shortcut_show_stations_list,                 showStationsList,               "showStationsList",                 "S");
        ShortcutManager::add(Shortcut::showTownsList,                   StringIds::shortcut_show_towns_list,                    showTownsList,                  "showTownsList",                    "U");
        ShortcutManager::add(Shortcut::showIndustriesList,              StringIds::shortcut_show_industries_list,               showIndustriesList,             "showIndustriesList",               "I");
        ShortcutManager::add(Shortcut::showMap,                         StringIds::shortcut_show_map,                           showMap,                        "showMap",                          "M");
        ShortcutManager::add(Shortcut::showCompaniesList,               StringIds::shortcut_show_companies_list,                showCompaniesList,              "showCompaniesList",                "C");
        ShortcutManager::add(Shortcut::showCompanyInformation,          StringIds::shortcut_show_company_information,           showCompanyInformation,         "showCompanyInformation",           "Q");
        ShortcutManager::add(Shortcut::showFinances,                    StringIds::shortcut_show_finances,                      showFinances,                   "showFinances",                     "F");
        ShortcutManager::add(Shortcut::showAnnouncementsList,           StringIds::shortcut_show_announcements_list,            showAnnouncementsList,          "showAnnouncementsList",            "Tab");
        ShortcutManager::add(Shortcut::screenshot,                      StringIds::shortcut_screenshot,                         makeScreenshot,                 "makeScreenshot",                   "Left Ctrl+S");
        ShortcutManager::add(Shortcut::toggleLastAnnouncement,          StringIds::shortcut_toggle_last_announcement,           toggleLastAnnouncement,         "toggleLastAnnouncement",           "Space");
        ShortcutManager::add(Shortcut::sendMessage,                     StringIds::shortcut_send_message,                       sendMessage,                    "sendMessage",                      "F1");
        ShortcutManager::add(Shortcut::constructionPreviousTab,         StringIds::shortcut_construction_previous_tab,          constructionPreviousTab,        "constructionPreviousTab",          "");
        ShortcutManager::add(Shortcut::constructionNextTab,             StringIds::shortcut_construction_next_tab,              constructionNextTab,            "constructionNextTab",              "");
        ShortcutManager::add(Shortcut::constructionPreviousTrackPiece,  StringIds::shortcut_construction_previous_track_piece,  constructionPreviousTrackPiece, "constructionPreviousTrackPiece",   "");
        ShortcutManager::add(Shortcut::constructionNextTrackPiece,      StringIds::shortcut_construction_next_track_piece,      constructionNextTrackPiece,     "constructionNextTrackPiece",       "");
        ShortcutManager::add(Shortcut::constructionPreviousSlope,       StringIds::shortcut_construction_previous_slope,        constructionPreviousSlope,      "constructionPreviousSlope",        "");
        ShortcutManager::add(Shortcut::constructionNextSlope,           StringIds::shortcut_construction_next_slope,            constructionNextSlope,          "constructionNextSlope",            "");
        ShortcutManager::add(Shortcut::constructionBuildAtCurrentPos,   StringIds::shortcut_construction_build_at_current_pos,  constructionBuildAtCurrentPos,  "constructionBuildAtCurrentPos",    "");
        ShortcutManager::add(Shortcut::constructionRemoveAtCurrentPos,  StringIds::shortcut_construction_remove_at_current_pos, constructionRemoveAtCurrentPos, "constructionRemoveAtCurrentPos",   "");
        ShortcutManager::add(Shortcut::constructionSelectPosition,      StringIds::shortcut_construction_select_position,       constructionSelectPosition,     "constructionSelectPosition",       "");
        ShortcutManager::add(Shortcut::gameSpeedNormal,                 StringIds::shortcut_game_speed_normal,                  gameSpeedNormal,                "gameSpeedNormal",                  "");
        ShortcutManager::add(Shortcut::gameSpeedFastForward,            StringIds::shortcut_game_speed_fast_forward,            gameSpeedFastForward,           "gameSpeedFastForward",             "");
        ShortcutManager::add(Shortcut::gameSpeedExtraFastForward,       StringIds::shortcut_game_speed_extra_fast_forward,      gameSpeedExtraFastForward,      "gameSpeedExtraFastForward",        "");
        ShortcutManager::add(Shortcut::honkAllHorns,                    StringIds::shortcut_game_speed_extra_fast_forward,      honkAllHorns,                   "honkAllHorns",                     "H");
        // clang-format on
    }
}
