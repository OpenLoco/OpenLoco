#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct cargo_object
    {
        string_id name; // 0x0
        uint16_t var_2;
        uint16_t var_4;
        string_id units_and_cargo_name; // 0x6
        string_id unit_name_singular;   // 0x8
        string_id unit_name_plural;     // 0xA
        uint32_t unit_inline_sprite;    // 0xC
        std::byte pad_10[0x12 - 0x10];
        uint8_t var_12;
        std::byte pad_13[0x1E - 0x13];
        uint8_t unitSize;
    };
#pragma pack(pop)

    static_assert(sizeof(cargo_object) == 0x1F);
    static_assert(0x00 == offsetof(cargo_object, name));
    static_assert(0x02 == offsetof(cargo_object, var_2));
    static_assert(0x04 == offsetof(cargo_object, var_4));
    static_assert(0x06 == offsetof(cargo_object, units_and_cargo_name));
    static_assert(0x08 == offsetof(cargo_object, unit_name_singular));
    static_assert(0x0A == offsetof(cargo_object, unit_name_plural));
    static_assert(0x0C == offsetof(cargo_object, unit_inline_sprite));
    static_assert(0x12 == offsetof(cargo_object, var_12));
}
