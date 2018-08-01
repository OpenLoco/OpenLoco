#pragma once

#include <cstdint>
#include <string>

namespace openloco::config
{
#pragma pack(push, 1)

    enum flags
    {
        gridlines_on_landscape = (1 << 0),
        show_height_as_units = (1 << 1),
        landscape_smoothing = (1 << 2),
        export_objects_with_saves = (1 << 3),

        preferred_currency_for_new_games = (1 << 6),
        preferred_currency_always = (1 << 7),

        use_preferred_owner_name = (1 << 9),
    };

    enum measurement_format
    {
        imperial = 0,
        metric = 1,
    };

    struct keyboard_shortcut_t
    {
        uint8_t var_0;
        uint8_t var_1;
    };

    struct config_t
    {
        uint32_t flags;             // 0x50AEB4, 0x00
        uint16_t resolution_width;  // 0x50AEB8, 0x04
        uint16_t resolution_height; // 0x50AEBA, 0x06
        uint8_t pad_08[0x0C - 0x08];
        uint8_t countdown; // 0x50AEC0, 0x0C
        uint8_t pad_0D[0x1F - 0x0D];
        uint32_t force_software_audio_mixer; // 0x1F
        uint8_t var_23;                      // 0x23
        uint8_t construction_marker;         // 0x50AED8, 0x24
        uint8_t pad_25[0x27 - 0x25];
        uint8_t sound_quality;      // 0x27
        uint8_t measurement_format; // 0x50AEDC, 0x28
        uint8_t pad_29;
        keyboard_shortcut_t keyboard_shortcuts[35]; // 0x2A
        uint8_t edge_scrolling;
        uint8_t vehicles_min_scale;    // 0x71
        uint8_t var_72;                // 0x50AF26, 0x72
        uint8_t var_73;                // 0x50AF27, 0x73
        uint16_t height_marker_offset; // 0x50AF28, 0x74
        uint8_t pad_76[0x7C - 0x76];
        uint8_t preferred_currency[0x10]; // 0x7C
        uint8_t enabled_music[29];        // 0x50AF40, 0x8C
        uint8_t pad_A9[0xCC - 0xA9];
        int32_t volume; // 0xCC
        uint8_t pad_D0[0xD4 - 0xD0];
        char last_host[64]; // 0xD4
        uint8_t station_names_min_scale;
        uint8_t var_115;
        char preferred_name[256]; // 0x116
    };
    static_assert(offsetof(config_t, keyboard_shortcuts) == 0x2A);
    static_assert(offsetof(config_t, preferred_name) == 0x116);
    static_assert(offsetof(config_t, last_host) == 0xD4);
    static_assert(sizeof(config_t) == 0x216);

    struct new_config
    {
        std::string loco_install_path;
        bool breakdowns_disabled = false;
    };

#pragma pack(pop)

    config_t& get();
    new_config& get_new();

    config_t& read();
    new_config& read_new_config();
    void write();
    void write_new_config();
}
