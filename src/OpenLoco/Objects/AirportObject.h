#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    struct airport_var_AE_object
    {
        int16_t x;      // 0x00
        int16_t y;      // 0x02
        int16_t z;      // 0x04
        uint16_t flags; // 0x06
    };

    struct airport_var_B2_object
    {
        uint8_t var_00;
        uint8_t var_01;
        uint8_t var_02;
        uint8_t var_03;
        uint32_t var_04;
        uint32_t var_08;
    };

    struct airport_object
    {
        string_id name;
        uint16_t build_cost_factor; // 0x02
        uint16_t sell_cost_factor;  // 0x04
        uint8_t cost_index;         //0x06
        uint8_t var_07;
        uint32_t image; // 0x08
        uint8_t pad_0C[0x10 - 0x0C];
        uint16_t allowed_plane_types; // 0x10
        uint8_t num_sprite_sets;      // 0x12
        uint8_t num_tiles;            // 0x13
        uint8_t pad_14[0xA0 - 0x14];
        uint32_t large_tiles;   // 0xA0
        int8_t min_x;           // 0xA4
        int8_t min_y;           // 0xA5
        int8_t max_x;           // 0xA6
        int8_t max_y;           // 0xA7
        uint16_t designed_year; // 0xA8
        uint16_t obsolete_year; // 0xAA
        uint8_t num_nodes;      // 0xAC
        uint8_t num_edges;      // 0xAD
        airport_var_AE_object* var_AE;
        airport_var_B2_object* var_B2;
        uint8_t pad_B6[0xBA - 0xB6];

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)
}
