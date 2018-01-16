#pragma once

#include <cstdint>
#include <limits>

namespace openloco
{
    using station_id_t = uint16_t;

    namespace station_id
    {
        constexpr station_id_t null = std::numeric_limits<station_id_t>::max();
    }

#pragma pack(push, 1)
    struct station_cargo_stats
    {
        int16_t status;
        int8_t pad_02[2];
        uint8_t rating;
        int8_t pad_05[8];

        bool empty() const
        {
            return status == -1;
        }
    };

    struct station
    {
        uint8_t pad_00[0x30];
        station_cargo_stats cargo[32];          // 0x30
        uint8_t pad_1D0[0x3D2 - 0x1D0];
    };
#pragma pack(pop)
}
