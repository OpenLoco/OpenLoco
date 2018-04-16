#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct currency_object
    {
        string_id name;
        string_id prefix_symbol;
        string_id suffix_symbol;
        uint32_t object_icon;
        uint8_t separator;
        uint8_t factor;
        // !!! TODO: verify object isn't larger.
    };
#pragma pack(pop)
}
