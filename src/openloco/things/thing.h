#pragma once

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
                uint8_t pad_00;
                uint8_t type;
                uint8_t pad_02;
                uint8_t pad_03;
                thing_id_t next_thing_id;   // 0x04
                uint8_t pad_06[0x0E - 0x06];
                uint16_t x;                 // 0x0E
                uint16_t y;                 // 0x10
                uint16_t z;                 // 0x12
                uint8_t pad_14[0x3A - 0x14];
                thing_id_t next_car_id;     // 0x3A
                uint8_t pad_3C[0x5F - 0x3C];
                uint8_t var_5D;
                uint8_t pad_5E;
                uint8_t var_5F;             // 0x5F (bit 1 = can break down)
                uint8_t pad_60[0x73 - 0x60];
                uint8_t var_73;             // 0x73 (bit 0 = broken down)
            };
        };
    };
#pragma pack(pop)
}
