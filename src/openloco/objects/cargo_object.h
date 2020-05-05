#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct cargo_object
    {
        string_id name;
        uint16_t var_02;
        uint16_t var_04;
        string_id var_06;
        string_id unit_name_singular; // 0x08
        string_id unit_name_plural;   // 0x0A
        uint32_t unit_inline_sprite;  // 0x0C
        uint16_t var_10;
        uint8_t var_12; // flags
        uint8_t var_13;
        uint8_t pad_14;
        uint8_t var_15;
        uint8_t var_16;
        uint8_t var_17;
        uint8_t pad_18[0x1B - 0x18];
        uint16_t var_1B;
        uint8_t var_1D; // currency related
        uint8_t var_1E;
    };
#pragma pack(pop)
}
