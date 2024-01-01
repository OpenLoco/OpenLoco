#pragma once

#include <cstdint>

namespace OpenLoco::Input
{
    enum class Shortcut : uint32_t
    {
        closeTopmostWindow,
        closeAllFloatingWindows,
        cancelConstructionMode,
        pauseUnpauseGame,
        zoomViewOut,
        zoomViewIn,
        rotateView,
        rotateConstructionObject,
        toggleUndergroundView,
        toggleSeeThroughTracks,
        toggleSeeThroughRoads,
        toggleSeeThroughTrees,
        toggleSeeThroughBuildings,
        toggleSeeThroughScenery,
        toggleHeightMarksOnLand,
        toggleHeightMarksOnTracks,
        toggleDirArrowsonTracks,
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
        screenshot,
        toggleLastAnnouncement,
        sendMessage,
        constructionPreviousTab,
        constructionNextTab,
        constructionPreviousTrackPiece,
        constructionNextTrackPiece,
        constructionPreviousSlope,
        constructionNextSlope,
        constructionBuildAtCurrentPos,
        constructionRemoveAtCurrentPos,
        constructionSelectPosition,
        gameSpeedNormal,
        gameSpeedFastForward,
        gameSpeedExtraFastForward,
    };

    namespace Shortcuts
    {
        void initialize();
    }
}
