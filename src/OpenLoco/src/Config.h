#pragma once

#include "Input.h"
#include "Objects/Object.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Input/ShortcutManager.h>
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

    struct Playlist
    {
        union
        {
            bool enabledMusic[29];
            struct
            {
                bool chuggin_along;
                bool long_dusty_road;
                bool flying_high;
                bool gettin_on_the_gas;
                bool jumpin_the_rails;
                bool smooth_running;
                bool traffic_jam;
                bool never_stop_til_you_get_there;
                bool soaring_away;
                bool techno_torture;
                bool everlasting_high_rise;
                bool solace;
                bool chrysanthemum;
                bool eugenia;
                bool the_ragtime_dance;
                bool easy_winners;
                bool setting_off;
                bool a_travellers_serenade;
                bool latino_trip;
                bool a_good_head_of_steam;
                bool hop_to_the_bop;
                bool the_city_lights;
                bool steamin_down_town;
                bool bright_expectations;
                bool mo_station;
                bool far_out;
                bool running_on_time;
                bool get_me_to_gladstone_bay;
                bool sandy_track_blues;
            };
        };
    };

    struct Audio
    {
        std::string device;
        int32_t mainVolume;
        bool playJukeboxMusic = true;
        bool playTitleMusic = true;
        bool playNewsSounds = true;
        MusicPlaylistType playlist;
        Playlist jukebox;
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

    constexpr auto messageCriticalityCount = 6;

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

        uint8_t constructionMarker;
        bool gridlinesOnLandscape = false;
        uint8_t heightMarkerOffset;
        bool landscapeSmoothing = true;
        bool showHeightAsUnits = false;
        uint8_t stationNamesMinScale = 2;
        uint8_t vehiclesMinScale = 2;

        bool allowMultipleInstances = false;
        bool cashPopupRendering = true;
        bool edgeScrolling = true;
        int32_t edgeScrollingSpeed = 12;
        bool zoomToCursor = true;
        NewsType newsSettings[messageCriticalityCount];

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

        bool usePreferredOwnerName = false;
        std::string preferredOwnerName;
        bool usePreferredOwnerFace;
        ObjectHeader preferredOwnerFace;

        uint8_t scenarioSelectedTab;

        std::map<Input::Shortcut, KeyboardShortcut> shortcuts;
    };

    Config& get();

    Config& read();
    void write();

    void resetShortcuts();
}
