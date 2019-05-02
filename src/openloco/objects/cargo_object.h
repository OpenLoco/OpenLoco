#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct cargo_object
    {
        string_id name;
        uint8_t pad_02[0x08 - 0x02];
        string_id unit_name_singular; // 0x08
        string_id unit_name_plural;   // 0x10
    };
#pragma pack(pop)
}
