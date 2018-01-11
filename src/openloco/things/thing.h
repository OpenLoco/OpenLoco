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
                uint8_t pad_06[0x3A - 0x06];
                thing_id_t next_car_id;     // 0x3A
            };
        };
    };
#pragma pack(pop)
}
