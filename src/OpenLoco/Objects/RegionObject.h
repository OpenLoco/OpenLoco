#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct region_object
    {
        string_id name;
        uint32_t image; // 0x02
    };
#pragma pack(pop)
}