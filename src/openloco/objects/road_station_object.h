#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct road_station_object
    {
        string_id name;
        uint8_t pad_02[0x0B - 0x02];
        uint8_t var_0B;
        uint8_t pad_0C[0x2C - 0x0C];
        uint8_t var_2C;
    };
#pragma pack(pop)
}
