#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    struct industry_object
    {
        string_id name;
        uint8_t pad_02[0x11 - 0x02];
        uint8_t var_11;
    };
}
