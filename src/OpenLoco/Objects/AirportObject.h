#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    namespace ApronAreaFlags
    {
        constexpr uint16_t terminal = (1 << 0);
        constexpr uint16_t takeoffEnd = (1 << 1);
        constexpr uint16_t flag2 = (1 << 2);
        constexpr uint16_t taxiing = (1 << 3);
        constexpr uint16_t inFlight = (1 << 4);
        constexpr uint16_t heliTakeoffBegin = (1 << 5);
        constexpr uint16_t takeoffBegin = (1 << 6);
        constexpr uint16_t heliTakeoffEnd = (1 << 7);
        constexpr uint16_t touchdown = (1 << 8);
    }

    struct AirportObjectApronArea
    {
        int16_t x;      // 0x00
        int16_t y;      // 0x02
        int16_t z;      // 0x04
        uint16_t flags; // 0x06 ApronAreaFlags
    };

    struct AirportObjectApronTransitions
    {
        uint8_t var_00;
        uint8_t curApronArea;  // 0x01
        uint8_t nextApronArea; // 0x02
        uint8_t var_03;
        uint32_t mustBeClearAreas;     // 0x04 Which areas must be clear to use the transition
        uint32_t atLeastOneClearAreas; // 0x08 Which areas must have at least one clear to use transition
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
        uint32_t large_tiles;                             // 0xA0
        int8_t min_x;                                     // 0xA4
        int8_t min_y;                                     // 0xA5
        int8_t max_x;                                     // 0xA6
        int8_t max_y;                                     // 0xA7
        uint16_t designed_year;                           // 0xA8
        uint16_t obsolete_year;                           // 0xAA
        uint8_t numApronAreas;                            // 0xAC
        uint8_t numApronTransitions;                      // 0xAD
        AirportObjectApronArea* apronAreas;               // 0xAE
        AirportObjectApronTransitions* apronTransistions; // 0xB2
        uint8_t pad_B6[0xBA - 0xB6];

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)
}
