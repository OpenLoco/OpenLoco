#pragma once

namespace openloco::input
{
    enum Shortcut
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
        toggleHideForegroundTracks,
        toggleHideForegroundScenery,
        toggleHeightMarksonLand,
        toggleHeightMarksonTracks,
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
        sendMessage
    };
}
