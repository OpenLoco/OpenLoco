#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    namespace flags_22
    {
        constexpr uint8_t unk_02 = 1 << 2;
    }
#pragma pack(push, 1)
    struct track_object
    {
        string_id name;
        uint8_t pad_02[0x1E - 0x02];
        uint32_t var_1E;
        uint16_t flags; // 0x22
    };
#pragma pack(pop)
}
