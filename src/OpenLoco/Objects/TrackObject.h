#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace Flags22
    {
        constexpr uint8_t unk_00 = 1 << 0;
        constexpr uint8_t unk_01 = 1 << 1;
        constexpr uint8_t unk_02 = 1 << 2;
    }

    namespace TrackPieceFlags
    {
        constexpr uint16_t diagonal = 1 << 0;
        constexpr uint16_t large_curve = 1 << 1;
        constexpr uint16_t normal_curve = 1 << 2;
        constexpr uint16_t small_curve = 1 << 3;
        constexpr uint16_t very_small_curve = 1 << 4;
        constexpr uint16_t slope = 1 << 5;
        constexpr uint16_t steep_slope = 1 << 6;
        constexpr uint16_t one_sided = 1 << 7;
        constexpr uint16_t sloped_curve = 1 << 8;
        constexpr uint16_t s_bend = 1 << 9;
        constexpr uint16_t junction = 1 << 10;
    }

#pragma pack(push, 1)
    struct TrackObject
    {
        static constexpr auto kObjectType = ObjectType::track;

        string_id name;
        uint16_t track_pieces;         // 0x02
        uint16_t station_track_pieces; // 0x04
        uint8_t var_06;
        uint8_t num_compatible; // 0x07
        uint8_t num_mods;       // 0x08
        uint8_t num_signals;    // 0x09
        uint8_t mods[4];        // 0x0A
        uint8_t var_0E;
        uint8_t pad_0F[0x14 - 0x0F];
        uint16_t build_cost_factor;  // 0x14
        uint16_t sell_cost_factor;   // 0x16
        uint16_t tunnel_cost_factor; // 0x18
        uint8_t cost_index;          // 0x1A
        uint8_t var_1B;
        uint16_t curve_speed;   // 0x1C
        uint32_t image;         // 0x1E
        uint16_t flags;         // 0x22
        uint8_t num_bridges;    // 0x24
        uint8_t bridges[7];     // 0x25
        uint8_t num_stations;   // 0x2C
        uint8_t stations[7];    // 0x2D
        uint8_t display_offset; // 0x34

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
