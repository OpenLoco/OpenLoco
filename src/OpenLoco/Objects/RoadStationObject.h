#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace RoadStationFlags
    {
        constexpr uint8_t recolourable = 1 << 0;
        constexpr uint8_t passenger = 1 << 1;
        constexpr uint8_t freight = 1 << 2;
        constexpr uint8_t roadEnd = 1 << 3;
    }

#pragma pack(push, 1)
    struct RoadStationObject
    {
        static constexpr auto kObjectType = ObjectType::roadStation;

        string_id name;
        uint8_t pad_02[0x04 - 0x02];
        uint16_t road_pieces;       // 0x04
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint8_t cost_index;         // 0x0A
        uint8_t flags;              // 0x0B
        uint32_t image;             // 0x0C
        uint8_t pad_10[0x20 - 0x10];
        uint8_t num_compatible; // 0x20
        uint8_t mods[7];        // 0x21
        uint16_t designed_year; // 0x28
        uint16_t obsolete_year; // 0x2A
        uint8_t var_2C;
        uint8_t pad_2D[0x6E - 0x2D];

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)

    namespace RoadStation::ImageIds
    {
        constexpr uint32_t preview_image = 0;
        constexpr uint32_t preview_image_windows = 1;
    }
}
