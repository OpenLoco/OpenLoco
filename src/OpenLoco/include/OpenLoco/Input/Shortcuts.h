#pragma once

#include "Input.h"
#include <cstdint>

namespace OpenLoco
{
    class FormatArguments;
}

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
        toggleSeeThroughBridges,
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
        showOptionsWindow,
        showJukeboxWindow,
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
        openDebugWindow,
    };

    constexpr uint32_t kInvalidKeyCode = 0xFFFFFFFF;

    struct KeyboardBinding
    {
        uint32_t keyCode = kInvalidKeyCode;
        KeyModifier modifiers = KeyModifier::invalid;
    };

    namespace Shortcuts
    {
        void initialize();

        void loadBindings();

        void resetBindings();

        const KeyboardBinding& getBinding(Shortcut id);

        void setBinding(Shortcut id, uint32_t keyCode, KeyModifier modifiers);

        void pushModifierStrings(FormatArguments& formatter, KeyModifier modifiers);
    }
}
