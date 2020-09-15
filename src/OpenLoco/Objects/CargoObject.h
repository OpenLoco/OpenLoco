#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct cargo_object
    {
        string_id name;
        uint8_t pad_02[0x06 - 0x02];
        string_id units_and_cargo_name; // 0x06
        string_id unit_name_singular;   // 0x08
        string_id unit_name_plural;     // 0x0A
        uint32_t unit_inline_sprite;    // 0x0C
    };
#pragma pack(pop)
}
