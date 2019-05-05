#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct land_object
    {
        string_id name;
        uint8_t pad_02[0x16 - 0x02];
        uint32_t var_16;
    };
#pragma pack(pop)

    namespace land::image_ids
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
