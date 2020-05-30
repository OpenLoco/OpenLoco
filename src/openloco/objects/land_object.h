#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct land_object
    {
        string_id name;
        uint8_t cost_index; // 0x02
        uint8_t pad_03[0x8 - 0x03];
        uint8_t cost_factor; // 0x08
        uint8_t pad_09[0x0A - 0x09];
        uint32_t image; // 0x0A
        uint8_t pad_0E[0x16 - 0x0E];
        uint32_t var_16;
    };
#pragma pack(pop)

    namespace land::image_ids
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
