#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct track_object
    {
        string_id name;
        uint8_t pad_02[0x1E - 0x02];
        uint32_t var_1E;
    };
#pragma pack(pop)
}
