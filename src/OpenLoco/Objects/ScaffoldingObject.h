#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct scaffolding_object
    {
        string_id name;
        uint32_t image; // 0x02
        uint8_t pad_06[0x12 - 0x06];
    };
#pragma pack(pop)
}
