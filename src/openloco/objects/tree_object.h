#pragma once

#include "../types.hpp"

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
        uint8_t num_rotations; // 0x06 (1,2,4)
        uint8_t growth;        // 0x07
        uint16_t var_08;       // 0x08
        uint32_t var_0A[12];   // 0x0A
        uint8_t pad_3A[0x3D - 0x3A];
        uint8_t var_3D;
        uint8_t states;
        uint8_t cost_index;   // 0x3F
        uint16_t cost_factor; // 0x40
        uint16_t var_42;
        uint32_t colours; // 0x44
        uint16_t var_48;
        uint16_t var_4A;
    };
#pragma pack(pop)
}
