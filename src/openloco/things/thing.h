#pragma once

#include "../types.hpp"
#include <cstdint>
#include <limits>

namespace openloco
{
    using thing_id_t = uint16_t;

    namespace thing_id
    {
        constexpr thing_id_t null = std::numeric_limits<thing_id_t>::max();
    }

#pragma pack(push, 1)
    struct thing
    {
        union
        {
            uint8_t pad_all[128];
            struct
            {
                uint8_t var_00;
                uint8_t type;
                uint8_t pad_02;
                uint8_t pad_03;
                thing_id_t next_thing_id; // 0x04
                uint8_t pad_06[0x09 - 0x06];
                uint8_t var_09;
                uint8_t pad_0A[0x0E - 0x0A];
                int16_t x; // 0x0E
                int16_t y; // 0x10
                int16_t z; // 0x12
                uint8_t var_14;
                uint8_t var_15;
                int16_t sprite_left;   // 0x16
                int16_t sprite_top;    // 0x18
                int16_t sprite_right;  // 0x1A
                int16_t sprite_bottom; // 0x1C
                uint8_t pad_1E[0x28 - 0x1E];
                uint16_t var_28;
                uint8_t pad_2A[0x2C - 0x2A];
                uint16_t var_2C;
                uint16_t var_2E;
                uint8_t pad_30[0x38 - 0x30];
                uint8_t var_38;
                uint8_t object_sprite_type; // 0x39
                thing_id_t next_car_id;     // 0x3A
                uint8_t pad_3C[0x40 - 0x3C];
                uint16_t object_type; // 0x40
                uint8_t var_42;
                uint8_t pad_43;
                int16_t var_44;
                uint8_t var_46;
                uint8_t pad_47[0x4C - 0x47];
                uint8_t cargo_type; // 0x4C
                uint8_t pad_4D;
                uint16_t cargo_origin; // 0x4E
                uint8_t pad_50;
                uint8_t cargo_quantity; // 0x51
                uint8_t pad_52[0x54 - 0x52];
                uint8_t var_54;
                uint8_t pad_55;
                uint32_t var_56;
                uint8_t pad_5A[0x5D - 0x5A];
                uint8_t var_5D;
                uint8_t var_5E;
                uint8_t var_5F; // 0x5F (bit 1 = can break down)
                uint8_t pad_60[0x6A - 0x60];
                uint8_t var_6A;
                uint8_t pad_6B[0x73 - 0x6B];
                uint8_t var_73; // 0x73 (bit 0 = broken down)
            };
        };

        void move_to(loc16 loc);
        void invalidate_sprite();
    };
#pragma pack(pop)
}
