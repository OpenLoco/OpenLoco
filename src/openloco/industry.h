#pragma once

#include "localisation/stringmgr.h"
#include "map/tile.h"
#include "objects/industry_object.h"
#include "utility/prng.hpp"
#include <cstdint>
#include <limits>

namespace openloco
{
    using namespace map;
    using industry_id_t = uint16_t;

    namespace industry_id
    {
        constexpr industry_id_t null = std::numeric_limits<industry_id_t>::max();
    }

    namespace industry_flags
    {
        constexpr uint16_t flag_01 = 1 << 0;
    }

#pragma pack(push, 1)
    struct industry
    {
        string_id name;
        int16_t var_02;
        int16_t var_04;
        uint16_t flags; // 0x06
        utility::prng prng;
        uint8_t object_id; // 0x10
        uint8_t pad_11[0xD5 - 0x11];
        uint16_t var_D5;
        int16_t tile_loop_x; // 0xD7
        int16_t tile_loop_y; // 0xD9
        int16_t var_DB;
        int16_t var_DD;
        uint8_t var_DF;
        uint8_t pad_E0[0x189 - 0xE0];
        uint16_t var_189;
        uint16_t var_18B;
        uint8_t pad_18D[0x1A3 - 0x18D];
        uint16_t var_1A3;
        uint16_t var_1A5;
        uint8_t pad_1A7[0x453 - 0x1A7];

        industry_id_t id() const;
        industry_object* object() const;
        bool empty() const;

        void update();
        void sub_454A43(coord_t x, coord_t y, uint8_t bl, uint8_t bh, uint8_t dl);
    };
#pragma pack(pop)
}
