#pragma once

#include "Input.h"
#include "Objects/Object.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace OpenLoco::Config
{
    enum class MeasurementFormat : uint8_t
    {
        imperial = 0,
        metric = 1,
    };

    enum class NewsType : uint8_t
    {
        none = 0,
        ticker,
        newsWindow,
    };

    enum class ScreenMode
    {
        window,
        fullscreen,
        fullscreenBorderless
    };

    enum class MusicPlaylistType : uint8_t
    {
        currentEra,
        all,
        custom,
    };

    struct Resolution
    {
        int32_t width{};
        int32_t height{};

        bool isPositive() const
        {
            return width > 0 && height > 0;
        }

        bool operator==(const Resolution& rhs) const
        {
            return width == rhs.width && height == rhs.height;
        }

        bool operator>(const Resolution& rhs) const
        {
            return width > rhs.width || height > rhs.height;
        }

        Resolution& operator*=(const float scalar)
        {
            width *= scalar;
            height *= scalar;
            return *this;
        }
    };

    struct Display
    {
        ScreenMode mode;
        int32_t index{};
        Resolution windowResolution = { 800, 600 };
        Resolution fullscreenResolution;
    };

    using Playlist = std::array<bool, 29>;

    struct Audio
    {
        std::string device;
        int32_t mainVolume = -1100;
        bool playJukeboxMusic = true;
        bool playTitleMusic = true;
        bool playNewsSounds = true;
        MusicPlaylistType playlist;
        Playlist customJukebox;
    };

    struct KeyboardShortcut
    {
        uint32_t keyCode;
        Input::KeyModifier modifiers;
    };

    struct Network
    {
        bool enabled{};
    };

    constexpr auto kMessageCriticalityCount = 6;

    // NB: int32_t is used for all numeric config variables for easier serialisation with yaml-cpp
    struct Config
    {
        Display display;
        Audio audio;
        Network network;
        std::string locoInstallPath;
        std::string lastSavePath;
        std::string lastLandscapePath;

        std::string language = "en-GB";
        MeasurementFormat measurementFormat;
        ObjectHeader preferredCurrency;
        bool usePreferredCurrencyForNewGames = false;
        bool usePreferredCurrencyAlways = false;

        float scaleFactor = 1.0f;
        bool showFPS = false;
        bool uncapFPS = false;

        int32_t constructionMarker;
        bool gridlinesOnLandscape = false;
        int32_t heightMarkerOffset;
        bool landscapeSmoothing = true;
        bool showHeightAsUnits = false;
        int32_t stationNamesMinScale = 2;
        int32_t vehiclesMinScale = 2;

        bool allowMultipleInstances = false;
        bool cashPopupRendering = true;
        bool edgeScrolling = true;
        int32_t edgeScrollingSpeed = 12;
        bool zoomToCursor = true;
        NewsType newsSettings[kMessageCriticalityCount];

        int32_t autosaveAmount = 12;
        int32_t autosaveFrequency = 1;
        bool exportObjectsWithSaves = true;

        bool breakdownsDisabled = false;
        bool buildLockedVehicles = false;
        bool cheatsMenuEnabled = false;
        bool companyAIDisabled = false;
        bool disableVehicleLoadPenaltyCheat = false;
        bool displayLockedVehicles = false;
        bool invertRightMouseViewPan = false;
        bool townGrowthDisabled = false;
        bool trainsReverseAtSignals = true;
        bool disableStationSizeLimit = false;
        bool showAiPlanningAsGhosts = false;

        bool usePreferredOwnerName = false;
        std::string preferredOwnerName;
        bool usePreferredOwnerFace;
        ObjectHeader preferredOwnerFace;

        int32_t scenarioSelectedTab;

        std::map<Input::Shortcut, KeyboardShortcut> shortcuts;
    };

    Config& get();

    Config& read();
    void write();

    void resetShortcuts();
}
