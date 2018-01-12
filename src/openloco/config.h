#pragma once

#include <cstdint>

namespace openloco::config
{
    #pragma pack(push, 1)

    struct config_t
    {
        uint32_t unk_00;
        uint16_t resolution_width;
        uint16_t resolution_height;
        uint8_t pad_06[526];
    };

    struct new_config
    {
        bool breakdowns_disabled = true;
    };

    #pragma pack(pop)

    config_t& get();
    new_config& get_new();

    void read();
    void write();
}
