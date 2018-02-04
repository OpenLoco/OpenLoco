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
        uint8_t colour_08;
        uint8_t pad_09[0x12 - 9];
        uint8_t colours_12[6];
    };
#pragma pack(pop)
}
