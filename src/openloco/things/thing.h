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
        uint8_t sprite_yaw;    // 0x1E
        uint8_t sprite_pitch;  // 0x1F

        void move_to(loc16 loc);
        void invalidate_sprite();
    };

    // Max size of a thing. Use when needing to know thing size
    struct any_thing : thing
    {
        uint8_t pad_20[128 - 0x20];
    };
#pragma pack(pop)
}
