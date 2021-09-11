#pragma once

#include "../Types.hpp"
#include "Object.h"
#include <array>

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct CompetitorObject
    {
        static constexpr auto _objectType = ObjectType::competitor;

        string_id var_00;        // 0x00
        string_id var_02;        // 0x02
        uint32_t var_04;         // 0x04
        uint32_t var_08;         // 0x08
        uint32_t emotions;       // 0x0C
        uint32_t images[9];      // 0x10
        uint8_t intelligence;    // 0x34
        uint8_t aggressiveness;  // 0x35
        uint8_t competitiveness; // 0x36
        uint8_t var_37;          // 0x37

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
    static_assert(sizeof(CompetitorObject) == 0x38);
#pragma pack(pop)

    [[nodiscard]] string_id aiRatingToLevel(const uint8_t rating);
}
