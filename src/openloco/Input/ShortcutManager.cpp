#include "ShortcutManager.h"
#include "../CompanyManager.h"
#include "../GameCommands.h"
#include "../Interop/interop.hpp"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../localisation/string_ids.h"
#include "../s5/s5.h"
#include "../ui/WindowManager.h"
#include <array>

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::input::ShortcutManager
{
    static constexpr size_t _count = 35;

    static void closeTopmostWindow();
    static void closeAllFloatingWindows();
    static void cancelConstructionMode();
    static void pauseUnpauseGame();
    static void zoomViewOut();
    static void zoomViewIn();
    static void rotateView();
    static void rotateConstructionObject();
    static void toggleUndergroundView();
    static void toggleHideForegroundTracks();
    static void toggleHideForegroundScenery();
    static void toggleHeightMarksOnLand();
    static void toggleHeightMarksOnTracks();
    static void toggleDirArrowsOnTracks();
    static void adjustLand();
    static void adjustWater();
    static void plantTrees();
    static void bulldozeArea();
    static void buildTracks();
    static void buildRoads();
    static void buildAirports();
    static void buildShipPorts();
    static void buildNewVehicles();
    static void showVehiclesList();
    static void showStationsList();
    static void showTownsList();
    static void showIndustriesList();
    static void showMap();
    static void showCompaniesList();
    static void showCompanyInformation();
    static void showFinances();
    static void showAnnouncementsList();
    static void makeScreenshot();
    static void toggleLastAnnouncement();
    static void sendMessage();

    static std::array<void (*)(), _count> _shortcuts = { {
        closeTopmostWindow,
        closeAllFloatingWindows,
        cancelConstructionMode,
        pauseUnpauseGame,
        zoomViewOut,
        zoomViewIn,
        rotateView,
        rotateConstructionObject,
        toggleUndergroundView,
        toggleHideForegroundTracks,
        toggleHideForegroundScenery,
        toggleHeightMarksOnLand,
        toggleHeightMarksOnTracks,
        toggleDirArrowsOnTracks,
        adjustLand,
        adjustWater,
        plantTrees,
        bulldozeArea,
        buildTracks,
        buildRoads,
        buildAirports,
        buildShipPorts,
        buildNewVehicles,
        showVehiclesList,
        showStationsList,
        showTownsList,
        showIndustriesList,
        showMap,
        showCompaniesList,
        showCompanyInformation,
        showFinances,
        showAnnouncementsList,
        makeScreenshot,
        toggleLastAnnouncement,
        sendMessage,
    } };

    static std::array<string_id, _count> _shortcutNames = { {
        string_ids::shortcut_close_topmost_window,
        string_ids::shortcut_close_all_floating_windows,
        string_ids::shortcut_cancel_construction_mode,
        string_ids::shortcut_pause_unpause_game,
        string_ids::shortcut_zoom_view_out,
        string_ids::shortcut_zoom_view_in,
        string_ids::shortcut_rotate_view,
        string_ids::shortcut_rotate_construction_object,
        string_ids::shortcut_toggle_underground_view,
        string_ids::shortcut_toggle_hide_foreground_tracks,
        string_ids::shortcut_toggle_hide_foreground_scenery,
        string_ids::shortcut_toggle_height_marks_on_land,
        string_ids::shortcut_toggle_height_marks_on_tracks,
        string_ids::shortcut_toggle_dir_arrows_on_tracks,
        string_ids::shortcut_adjust_land,
        string_ids::shortcut_adjust_water,
        string_ids::shortcut_plant_trees,
        string_ids::shortcut_bulldoze_area,
        string_ids::shortcut_build_tracks,
        string_ids::shortcut_build_roads,
        string_ids::shortcut_build_airports,
        string_ids::shortcut_build_ship_ports,
        string_ids::shortcut_build_new_vehicles,
        string_ids::shortcut_show_vehicles_list,
        string_ids::shortcut_show_stations_list,
        string_ids::shortcut_show_towns_list,
        string_ids::shortcut_show_industries_list,
        string_ids::shortcut_show_map,
        string_ids::shortcut_show_companies_list,
        string_ids::shortcut_show_company_information,
        string_ids::shortcut_show_finances,
        string_ids::shortcut_show_announcements_list,
        string_ids::shortcut_screenshot,
        string_ids::shortcut_toggle_last_announcement,
        string_ids::shortcut_send_message,
    } };

    size_t count()
    {
        return _count;
    }

    void execute(Shortcut s)
    {
        _shortcuts[s]();
    }

    string_id getName(Shortcut s)
    {
        return _shortcutNames[s];
    }

    // 0x004BF089
    static void closeTopmostWindow()
    {
        WindowManager::closeTopmost();
    }

    // 004BF0B6
    static void closeAllFloatingWindows()
    {
        WindowManager::closeAllFloatingWindows();
    }

    // 0x4BF0BC
    static void cancelConstructionMode()
    {
        call(0x004BF0BC);
    }

    // 0x4BF0E6
    static void pauseUnpauseGame()
    {
        if (isEditorMode())
            return;

        game_commands::do_20();
    }

    // 0x004BF0FE
    static void zoomViewOut()
    {
        window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportZoomOut(false);
        townmgr::updateLabels();
        stationmgr::updateLabels();
    }

    // 0x004BF115
    static void zoomViewIn()
    {
        window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportZoomIn(false);
        townmgr::updateLabels();
        stationmgr::updateLabels();
    }

    // 0x004BF12C
    static void rotateView()
    {
        window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportRotateRight();
        townmgr::updateLabels();
        stationmgr::updateLabels();
        windows::map::centerOnViewPoint();
    }

    // 0x004BF148
    static void rotateConstructionObject()
    {
        call(0x004BF148);
    }

    // 0x004BF18A
    static void toggleUndergroundView()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= viewport_flags::underground_view;
        window->invalidate();
    }

    // 0x004BF194
    static void toggleHideForegroundTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= viewport_flags::hide_foreground_tracks_roads;
        window->invalidate();
    }

    // 0x004BF19E
    static void toggleHideForegroundScenery()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= viewport_flags::hide_foreground_scenery_buildings;
        window->invalidate();
    }

    // 0x004BF1A8
    static void toggleHeightMarksOnLand()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= viewport_flags::height_marks_on_land;
        window->invalidate();
    }

    // 0x004BF1B2
    static void toggleHeightMarksOnTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= viewport_flags::height_marks_on_tracks_roads;
        window->invalidate();
    }

    // 0x004BF1BC
    static void toggleDirArrowsOnTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= viewport_flags::one_way_direction_arrows;
        window->invalidate();
    }

    // 0x004BF1C6
    static void adjustLand()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::terraform::openAdjustLand();
    }

    // 0x004BF1E1
    static void adjustWater()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::terraform::openAdjustWater();
    }

    // 0x004BF1FC
    static void plantTrees()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::terraform::openPlantTrees();
    }

    // 0x004BF217
    static void bulldozeArea()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::terraform::openClearArea();
    }

    // 0x004BF232
    static void buildTracks()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAA> last_railroad_option;
        if (last_railroad_option == 0xFF)
            return;

        windows::construction::openWithFlags(*last_railroad_option);
    }

    // 0x004BF24F
    static void buildRoads()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        loco_global<uint8_t, 0x00525FAB> last_road_option;
        if (last_road_option == 0xFF)
            return;

        windows::construction::openWithFlags(*last_road_option);
    }

    // 0x004BF276
    static void buildAirports()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAC> have_airports;
        if (have_airports == 0xFF)
            return;

        windows::construction::openWithFlags(1 << 31);
    }

    // 0x004BF295
    static void buildShipPorts()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAD> have_ship_ports;
        if (have_ship_ports == 0xFF)
            return;

        windows::construction::openWithFlags(1 << 30);
    }

    // 0x004BF2B4
    static void buildNewVehicles()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x0052622C> last_build_vehicles_option;
        if (last_build_vehicles_option == 0xFF)
            return;

        BuildVehicle::open(last_build_vehicles_option, 1 << 31);
    }

    // 0x004BF2D1
    static void showVehiclesList()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAF> last_vehicles_option;
        windows::vehicle_list::open(companymgr::getControllingId(), *last_vehicles_option);
    }

    // 0x004BF2F0
    static void showStationsList()
    {
        if (isEditorMode())
            return;

        windows::station_list::open(companymgr::getControllingId(), 0);
    }

    // 0x004BF308
    static void showTownsList()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::town_list::open();
    }

    // 0x004BF323
    static void showIndustriesList()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::industry_list::open();
    }

    // 0x004BF33E
    static void showMap()
    {
        if (isEditorMode() && s5::getOptions().editorStep == 0)
            return;

        windows::map::open();
    }

    // 0x004BF359
    static void showCompaniesList()
    {
        call(0x004BF359);
    }

    // 0x004BF36A
    static void showCompanyInformation()
    {
        call(0x004BF36A);
    }

    // 0x004BF382
    static void showFinances()
    {
        if (isEditorMode())
            return;

        windows::CompanyWindow::openFinances(companymgr::getControllingId());
    }

    // 0x004BF39A
    static void showAnnouncementsList()
    {
        if (isEditorMode())
            return;

        MessageWindow::open();
    }

    // 0x004BF3AB
    static void makeScreenshot()
    {
        call(0x004BF3AB);
    }

    // 0x004BF3B3
    static void toggleLastAnnouncement()
    {
        call(0x004BF3B3);
    }

    // 0x004BF3DC
    static void sendMessage()
    {
        call(0x004BF3DC);
    }
}
