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
    struct CurrencyObject
    {
        static constexpr auto kObjectType = ObjectType::currency;

        string_id name;          // 0x00
        string_id prefix_symbol; // 0x02
        string_id suffix_symbol; // 0x04
        uint32_t object_icon;    // 0x06
        uint8_t separator;       // 0x0A
        uint8_t factor;          // 0x0B
        // !!! TODO: verify object isn't larger.

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
