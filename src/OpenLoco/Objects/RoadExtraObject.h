#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct RoadExtraObject
    {
        static constexpr auto kObjectType = ObjectType::roadExtra;

        string_id name;
        uint16_t road_pieces;       // 0x02
        uint8_t is_overhead;        // 0x04
        uint8_t cost_index;         // 0x05
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint32_t image;             // 0x0A
        uint32_t var_0E;

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
