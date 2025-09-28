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
#pragma pack(push, 1)

    enum class Flags : uint32_t
    {
        none = 0U,
        gridlinesOnLandscape = 1U << 0,   // unused
        showHeightAsUnits = 1U << 1,      // unused
        landscapeSmoothing = 1U << 2,     // unused
        exportObjectsWithSaves = 1U << 3, // unused

        preferredCurrencyForNewGames = 1U << 6, // unused
        preferredCurrencyAlways = 1U << 7,      // unused

        usePreferredOwnerName = 1U << 9, // unused
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags);

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

    constexpr auto messageCriticalityCount = 6;

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

    struct LocoConfig
    {
        // This struct has been deprecated; please use Config::KeyboardShortcut instead.
        struct KeyboardShortcut
        {
            uint8_t var_0;
            uint8_t var_1;
        };

        Flags flags;                                    // 0x50AEB4, 0x00
        int16_t resolutionWidth;                        // 0x50AEB8, 0x04
        int16_t resolutionHeight;                       // 0x50AEBA, 0x06
        uint16_t backupResolutionWidth;                 // 0x50AEBC, 0x08
        uint16_t backupResolutionHeight;                // 0x50AEBE, 0x10
        uint8_t countdown;                              // 0x50AEC0, 0x0C
        bool var_0D;                                    // 0x0D
        uint8_t audioDeviceGuid[16];                    // 0x0E
        uint8_t var_1E;                                 // 0x1E
        uint32_t forceSoftwareAudioMixer;               // 0x1F
        uint8_t musicPlaying;                           // 0x23
        uint8_t constructionMarker;                     // 0x50AED8, 0x24
        uint8_t maxVehicleSounds;                       // 0x25
        uint8_t maxSoundInstances;                      // 0x26
        uint8_t soundQuality;                           // 0x27
        MeasurementFormat measurementFormat;            // 0x50AEDC, 0x28
        uint8_t pad_29;                                 // 0x29
        KeyboardShortcut keyboardShortcuts[35];         // 0x2A
        uint8_t edgeScrolling;                          // 0x70
        uint8_t vehiclesMinScale;                       // 0x50AF25, 0x71
        uint8_t var_72;                                 // 0x50AF26, 0x72
        MusicPlaylistType musicPlaylist;                // 0x50AF27, 0x73
        uint16_t heightMarkerOffset;                    // 0x50AF28, 0x74
        NewsType newsSettings[messageCriticalityCount]; // 0x50AF2A, 0x76
        ObjectHeader preferredCurrency;                 // 0x7C
        uint8_t enabledMusic[29];                       // 0x50AF40, 0x8C
        uint8_t pad_A9[0xCC - 0xA9];                    // 0xA9
        int32_t volume;                                 // 0x50AF80, 0xCC
        uint32_t connectionTimeout;                     // 0xD0
        char lastHost[64];                              // 0xD4
        uint8_t stationNamesMinScale;                   // 0x114
        uint8_t scenarioSelectedTab;                    // 0x115
        char preferredName[256];                        // 0x116
    };
    static_assert(offsetof(LocoConfig, keyboardShortcuts) == 0x2A);
    static_assert(offsetof(LocoConfig, preferredName) == 0x116);
    static_assert(offsetof(LocoConfig, lastHost) == 0xD4);
    static_assert(sizeof(LocoConfig) == 0x216);

#pragma pack(pop)

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

    struct NewConfig
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

        bool gridlinesOnLandscape = false;
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

        LocoConfig old;
    };

    NewConfig& get();

    NewConfig& read();
    void write();

    void resetShortcuts();
}
