#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct water_object
    {
        string_id name;
        uint8_t pad_02[0x06 - 0x02];
        uint32_t var_06;
    };
#pragma pack(pop)

    namespace water::image_ids
    {
        constexpr uint32_t toolbar_terraform_water = 42;
    }
}
