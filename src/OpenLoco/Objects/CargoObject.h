#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace CargoObjectFlags
    {
        constexpr uint8_t refit = (1 << 1);
    }

#pragma pack(push, 1)
    struct CargoObject
    {
        static constexpr auto kObjectType = ObjectType::cargo;

        string_id name; // 0x0
        uint16_t var_2;
        uint16_t var_4;
        string_id units_and_cargo_name; // 0x6
        string_id unit_name_singular;   // 0x8
        string_id unit_name_plural;     // 0xA
        uint32_t unit_inline_sprite;    // 0xC
        std::uint8_t pad_10[0x12 - 0x10];
        uint8_t flags; // 0x12
        std::uint8_t pad_13;
        uint8_t var_14;
        uint8_t var_15;
        uint8_t var_16;
        uint16_t var_17;
        uint16_t var_19;
        uint16_t var_1B;
        uint8_t var_1D;
        uint8_t unitSize; // 0x1E
    };
#pragma pack(pop)

    static_assert(sizeof(CargoObject) == 0x1F);
}
