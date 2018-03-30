#pragma once

#include <cstdint>
#include <string>

namespace openloco::config
{
#pragma pack(push, 1)

    struct config_t
    {
        uint8_t show_height_as_units;
        uint8_t pad_01[3];
        uint16_t resolution_width;
        uint16_t resolution_height;
        uint8_t pad_04[32];
        uint8_t measurement_format;
        uint8_t pad_06[493];
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
