#include "ShortcutManager.h"
#include "../game_commands.h"
#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"
#include <array>

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::input::ShortcutManager
{
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

    static std::array<void (*)(), 35> _shortcuts = { {
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

    void execute(Shortcut s)
    {
        _shortcuts[s]();
    }

    // 0x004BF089
    static void closeTopmostWindow()
    {
        WindowManager::closeTopmost();
    }

    // 004BF0B6
    static void closeAllFloatingWindows()
    {
        call(0x004CF456);
    }

    // 0x4BF0BC
    static void cancelConstructionMode()
    {
        call(0x004BF0BC);
    }

    // 0x4BF0E6
    static void pauseUnpauseGame()
    {
        if (is_editor_mode())
            return;

        registers regs;
        regs.bl = 1;
        game_commands::do_command(20, regs);
    }

    // 0x004BF0FE
    static void zoomViewOut()
    {
        call(0x004BF0FE);
    }

    // 0x004BF115
    static void zoomViewIn()
    {
        call(0x004BF115);
    }

    // 0x004BF12C
    static void rotateView()
    {
        call(0x004BF12C);
    }

    // 0x004BF148
    static void rotateConstructionObject()
    {
        call(0x004BF148);
    }

    // 0x004BF18A
    static void toggleUndergroundView()
    {
        call(0x004BF18A);
    }

    // 0x004BF194
    static void toggleHideForegroundTracks()
    {
        call(0x004BF194);
    }

    // 0x004BF19E
    static void toggleHideForegroundScenery()
    {
        call(0x004BF19E);
    }

    // 0x004BF1A8
    static void toggleHeightMarksOnLand()
    {
        call(0x004BF1A8);
    }

    // 0x004BF1B2
    static void toggleHeightMarksOnTracks()
    {
        call(0x004BF1B2);
    }

    // 0x004BF1BC
    static void toggleDirArrowsOnTracks()
    {
        call(0x004BF1BC);
    }

    // 0x004BF1C6
    static void adjustLand()
    {
        call(0x004BF1C6);
    }

    // 0x004BF1E1
    static void adjustWater()
    {
        call(0x004BF1E1);
    }

    // 0x004BF1FC
    static void plantTrees()
    {
        call(0x004BF1FC);
    }

    // 0x004BF217
    static void bulldozeArea()
    {
        call(0x004BF217);
    }

    // 0x004BF232
    static void buildTracks()
    {
        call(0x004BF232);
    }

    // 0x004BF24F
    static void buildRoads()
    {
        call(0x004BF24F);
    }

    // 0x004BF276
    static void buildAirports()
    {
        call(0x004BF276);
    }

    // 0x004BF295
    static void buildShipPorts()
    {
        call(0x004BF295);
    }

    // 0x004BF2B4
    static void buildNewVehicles()
    {
        call(0x004BF2B4);
    }

    // 0x004BF2D1
    static void showVehiclesList()
    {
        call(0x004BF2D1);
    }

    // 0x004BF2F0
    static void showStationsList()
    {
        call(0x004BF2F0);
    }

    // 0x004BF308
    static void showTownsList()
    {
        call(0x004BF308);
    }

    // 0x004BF323
    static void showIndustriesList()
    {
        call(0x004BF323);
    }

    // 0x004BF33E
    static void showMap()
    {
        call(0x004BF33E);
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
        call(0x004BF382);
    }

    // 0x004BF39A
    static void showAnnouncementsList()
    {
        call(0x004BF39A);
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
