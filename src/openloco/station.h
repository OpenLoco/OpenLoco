#pragma once

#include "localisation/stringmgr.h"
#include "town.h"
#include "utility/numeric.hpp"
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
        uint8_t var_39;
        uint8_t pad_40;

        bool empty() const
        {
            return origin == station_id::null;
        }

        bool is_accepted() const
        {
            return flags & 1;
        }

        void is_accepted(bool value)
        {
            flags = utility::set_mask<uint8_t>(flags, 1, value);
        }
    };

    constexpr size_t max_cargo_stats = 32;

    struct station
    {
        string_id name; // 0x00
        uint8_t pad_02[0x08 - 0x02];
        uint16_t var_08[4];
        uint16_t var_10[4];
        uint16_t var_18[4];
        uint16_t var_20[4];
        company_id_t owner; // 0x28
        uint8_t var_29;
        uint16_t var_2A;
        town_id_t town;                                   // 0x2C
        station_cargo_stats cargo_stats[max_cargo_stats]; // 0x2E
        uint16_t var_1CE;
        uint8_t pad_1D0[0x3B0 - 0x1D0];
        uint8_t var_3B0;
        uint8_t var_3B1;
        uint8_t pad_3B2[0x3D2 - 0x3B2];

        bool empty() const { return name == string_ids::null; }
        station_id_t id() const;
        void update();
        uint32_t calc_accepted_cargo(uint16_t ax = 0xFFFF);
        void sub_48F7D1();
        bool update_cargo();
        int32_t calculate_cargo_rating(const station_cargo_stats& cargo) const;
        void invalidate();
        void invalidate_window();

    private:
        void sub_4929DB();
    };
#pragma pack(pop)
}
