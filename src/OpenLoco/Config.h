#pragma once

#include "Input/ShortcutManager.h"
#include "Objects/ObjectManager.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace OpenLoco::Config
{
#pragma pack(push, 1)

    namespace Flags
    {
        constexpr uint32_t gridlinesOnLandscape = (1 << 0);
        constexpr uint32_t showHeightAsUnits = (1 << 1);
        constexpr uint32_t landscapeSmoothing = (1 << 2);
        constexpr uint32_t exportObjectsWithSaves = (1 << 3);

        constexpr uint32_t preferredCurrencyForNewGames = (1 << 6);
        constexpr uint32_t preferredCurrencyAlways = (1 << 7);

        constexpr uint32_t usePreferredOwnerName = (1 << 9);
    }

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

        uint32_t flags;                                  // 0x50AEB4, 0x00
        int16_t resolution_width;                        // 0x50AEB8, 0x04
        int16_t resolution_height;                       // 0x50AEBA, 0x06
        uint16_t backup_resolution_width;                // 0x50AEBC, 0x08
        uint16_t backup_resolution_height;               // 0x50AEBE, 0x10
        uint8_t countdown;                               // 0x50AEC0, 0x0C
        bool var_0D;                                     // 0x0D
        uint8_t audio_device_guid[16];                   // 0x0E
        uint8_t var_1E;                                  // 0x1E
        uint32_t force_software_audio_mixer;             // 0x1F
        uint8_t music_playing;                           // 0x23
        uint8_t construction_marker;                     // 0x50AED8, 0x24
        uint8_t max_vehicle_sounds;                      // 0x25
        uint8_t max_sound_instances;                     // 0x26
        uint8_t sound_quality;                           // 0x27
        MeasurementFormat measurement_format;            // 0x50AEDC, 0x28
        uint8_t pad_29;                                  // 0x29
        KeyboardShortcut keyboard_shortcuts[35];         // 0x2A
        uint8_t edge_scrolling;                          // 0x70
        uint8_t vehicles_min_scale;                      // 0x50AF25, 0x71
        uint8_t var_72;                                  // 0x50AF26, 0x72
        MusicPlaylistType music_playlist;                // 0x50AF27, 0x73
        uint16_t height_marker_offset;                   // 0x50AF28, 0x74
        NewsType news_settings[messageCriticalityCount]; // 0x50AF2A, 0x76
        ObjectHeader preferred_currency;                 // 0x7C
        uint8_t enabled_music[29];                       // 0x50AF40, 0x8C
        uint8_t pad_A9[0xCC - 0xA9];                     // 0xA9
        int32_t volume;                                  // 0x50AF80, 0xCC
        uint32_t connection_timeout;                     // 0xD0
        char last_host[64];                              // 0xD4
        uint8_t station_names_min_scale;                 // 0x114
        uint8_t scenario_selected_tab;                   // 0x115
        char preferred_name[256];                        // 0x116
    };
    static_assert(offsetof(LocoConfig, keyboard_shortcuts) == 0x2A);
    static_assert(offsetof(LocoConfig, preferred_name) == 0x116);
    static_assert(offsetof(LocoConfig, last_host) == 0xD4);
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

        bool operator!=(const Resolution& rhs) const
        {
            return width != rhs.width || height != rhs.height;
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
        Resolution window_resolution = { 800, 600 };
        Resolution fullscreen_resolution;
    };

    struct Audio
    {
        std::string device;
        bool play_title_music = true;
    };

    struct KeyboardShortcut
    {
        uint32_t keyCode;
        uint8_t modifiers;
    };

    struct NewConfig
    {
        Display display;
        Audio audio;
        std::string loco_install_path;
        std::string last_save_path;
        std::string language = "en-GB";
        bool cheats_menu_enabled = false;
        bool breakdowns_disabled = false;
        bool trainsReverseAtSignals = true;
        bool companyAIDisabled = false;
        float scale_factor = 1.0f;
        bool zoom_to_cursor = true;
        int32_t autosave_frequency = 1;
        int32_t autosave_amount = 12;
        bool showFPS = false;
        bool uncapFPS = false;
        KeyboardShortcut shortcuts[Input::ShortcutManager::count];
    };

    LocoConfig& get();
    NewConfig& getNew();

    LocoConfig& read();
    NewConfig& readNewConfig();
    void write();
    void writeNewConfig();

    void resetShortcuts();
}
