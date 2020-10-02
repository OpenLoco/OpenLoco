#pragma once

#include "../Types.hpp"
#include <array>

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

    const std::array<uint8_t, 11> treeGrowth = { {
        1,
        0,
        1,
        2,
        2,
        3,
        4,
        5,
        6,
        0,
        0,
    } };

#pragma pack(push, 1)
    struct tree_object
    {
        string_id name;
        uint8_t var_02;
        uint8_t height; // 0x03
        uint8_t var_04;
        uint8_t var_05;
        uint8_t num_rotations; // 0x06 (1,2,4)
        uint8_t growth;        // 0x07 (number of tree size images)
        uint16_t var_08;       // 0x08
        uint32_t sprites[12];  // 0x0A
        uint8_t pad_3A[0x3D - 0x3A];
        uint8_t season_state; // 0x3D (index for sprites, seasons + dying)
        uint8_t var_3E;
        uint8_t cost_index;         // 0x3F
        uint16_t build_cost_factor; // 0x40
        uint16_t clear_cost_factor; // 0x42
        uint32_t colours;           // 0x44
        uint16_t var_48;
        uint16_t var_4A;

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y);
    };
#pragma pack(pop)
}
