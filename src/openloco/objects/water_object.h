#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct water_object
    {
        string_id name;
        uint8_t cost_index; // 0x02
        uint8_t var_03;
        uint8_t cost_factor; //0x04
        uint8_t var_05;
        uint32_t var_06;
    };
#pragma pack(pop)

    namespace water::image_ids
    {
        constexpr uint32_t toolbar_terraform_water = 42;
    }
}
