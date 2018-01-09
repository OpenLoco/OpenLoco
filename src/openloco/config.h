#pragma once

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

    #pragma pack(pop)

    config_t& get();

    void read();
    void write();
}
