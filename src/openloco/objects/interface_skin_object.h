#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct interface_skin_object
    {
        string_id name;
        uint32_t img;
        uint8_t colours_06[2];
        uint8_t pad_08[0x12 - 8];
        uint8_t colours_12[6];
    };
#pragma pack(pop)
}
