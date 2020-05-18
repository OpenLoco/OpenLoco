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
        uint8_t num_rotations;
        uint8_t var_07;
        uint16_t var_08;
        uint8_t pad_0A[0x3D - 0x0A];
        uint8_t var_3D;
        uint8_t var_3E;
        uint8_t cost_index;     // 0x3F
        uint16_t cost_factor;   // 0x40
        uint16_t var_42;
        uint32_t colours; // 0x44
        uint16_t var_48;
        uint16_t var_4A;
    };
#pragma pack(pop)
}