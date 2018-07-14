#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct tree_object
    {
        string_id name;
        uint8_t var_02;
        uint8_t var_03;
        uint8_t var_04;
        uint8_t var_05;
        uint8_t rotation_count; // 0x06
        uint8_t growth_stages;  // 0x07
        uint16_t var_08;
        uint32_t image_ids[2][6]; // 0x0A
        uint16_t shadow_image_offset;
        uint8_t var_3C;
        uint8_t pad_3D;
        uint8_t var_3E;
        uint8_t var_3F;
        uint16_t var_40;
        uint16_t var_42;
        uint32_t var_44;
        uint16_t var_48;
        uint8_t pad_4A[0x4C - 0x4A];
    };
    static_assert(sizeof(tree_object) == 0x4C);
#pragma pack(pop)
}
