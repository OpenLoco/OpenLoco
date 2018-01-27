#pragma once

#include "localisation/stringmgr.h"
#include <cstdint>
#include <limits>

namespace openloco
{
    using industry_id_t = uint16_t;

    namespace industry_id
    {
        constexpr industry_id_t null = std::numeric_limits<industry_id_t>::max();
    }

#pragma pack(push, 1)
    struct industry
    {
        string_id name;
        uint8_t pad_02[0xD5 - 0x02];
        uint16_t var_D5;
        uint8_t pad_D7[0x453 - 0xD7];

        bool empty() const;
    };
#pragma pack(pop)
}
