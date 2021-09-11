#pragma once

#include "../Map/Map.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct DockObject
    {
        static constexpr auto _objectType = ObjectType::dock;

        string_id name;
        uint16_t build_cost_factor; // 0x02
        uint16_t sell_cost_factor;  // 0x04
        uint8_t cost_index;         // 0x06
        uint8_t var_07;
        uint32_t image; // 0x08
        uint8_t pad_0C[0x12 - 0x0C];
        uint8_t num_aux_01;     // 0x12
        uint8_t num_aux_02_ent; // 0x13
        uint8_t pad_14[0x20 - 0x14];
        uint16_t designed_year; // 0x20
        uint16_t obsolete_year; // 0x22
        Map::Pos2 boatPosition; // 0x24

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)
}
