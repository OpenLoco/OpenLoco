#pragma once

#include <cstdint>
#include <string>

namespace openloco::config
{
#pragma pack(push, 1)

    enum flags
    {
        show_height_as_units = (1 << 1),
    };

    enum measurement_format
    {
        imperial = 0,
        metric = 1,
    };

    struct config_t
    {
        uint32_t flags;             // 0x50AEB4, 0x00
        uint16_t resolution_width;  // 0x50AEB8, 0x04
        uint16_t resolution_height; // 0x50AEBA, 0x06
        uint8_t pad_08[0x24 - 0x08];
        uint8_t var_24;
        uint8_t pad_25[0x28 - 0x25];
        uint8_t measurement_format; // 0x50AEDC, 0x28
        uint8_t pad_29;
        uint8_t keyboard_shortcuts[0xAF24 - 0xAEDE]; // 0x30
        uint8_t pad_71[0xB0CA - 0xAF25];
    };

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
