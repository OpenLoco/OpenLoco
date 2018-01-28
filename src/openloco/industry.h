#pragma once

#include "localisation/stringmgr.h"
#include "objects/industry_object.h"
#include <cstdint>
#include <limits>

namespace openloco
{
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
        uint8_t pad_02[0x06 - 0x02];
        uint16_t flags; // 0x06
        uint8_t pad_08[0x10 - 0x08];
        uint8_t object_id; // 0x10
        uint8_t pad_11[0xD5 - 0x11];
        uint16_t var_D5;
        int16_t tile_loop_x; // 0xD7
        int16_t tile_loop_y; // 0xD9
        int16_t var_DB;
        int16_t var_DD;
        uint8_t pad_DF[0x453 - 0xDD];
        industry_id_t id() const;
        industry_object* object() const;
        bool empty() const;
        void update();

    };
#pragma pack(pop)
}
