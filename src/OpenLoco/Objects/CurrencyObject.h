#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    struct currency_object
    {
        string_id name;          // 0x00
        string_id prefix_symbol; // 0x02
        string_id suffix_symbol; // 0x04
        uint32_t object_icon;    // 0x06
        uint8_t separator;       // 0x0A
        uint8_t factor;          // 0x0B
        // !!! TODO: verify object isn't larger.

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y);
    };
#pragma pack(pop)
}
