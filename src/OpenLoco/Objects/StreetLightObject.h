#pragma once

#include "../Localisation/StringManager.h"

namespace openloco
{
#pragma pack(push, 1)
    struct street_light_object
    {
        string_id name;
        uint16_t designedYear[3]; // 0x02
        uint32_t image;           // 0x08
    };
#pragma pack(pop)
}
