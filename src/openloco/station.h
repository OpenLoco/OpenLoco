#pragma once

#include "localisation/stringmgr.h"
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
        uint16_t quantity;   // 0x2E
        station_id_t origin; // 0x30
        uint8_t flags;       // 0x32
        uint8_t age;         // 0x33
        uint8_t rating;      // 0x34
        uint8_t enroute_age; // 0x35
        uint16_t var_36;     // 0x36
        uint8_t var_38;
        uint8_t pad_39[2];

        bool empty() const
        {
            return origin == station_id::null;
        }

        bool is_accepted() const
        {
            return flags & 1;
        }
    };

    struct station
    {
        string_id name; // 0x00
        uint8_t pad_02[0x28 - 0x02];
        uint8_t var_28;
        uint8_t pad_29[0x2A - 0x29];
        uint16_t var_2A;
        string_id town_name;                 // 0x2C
        station_cargo_stats cargo_stats[32]; // 0x2E
        uint8_t pad_1CE[0x3B0 - 0x1CE];
        uint8_t var_3B0;
        uint8_t var_3B1;
        uint8_t pad_3B2[0x3D2 - 0x3B2];

        station_id_t id();
        bool update_cargo();
        int32_t calculate_cargo_rating(const station_cargo_stats& cargo) const;

    private:
        void sub_4929DB();
    };
#pragma pack(pop)
}
