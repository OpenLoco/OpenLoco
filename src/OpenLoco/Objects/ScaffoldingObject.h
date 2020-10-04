#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    struct scaffolding_object
    {
        string_id name;
        uint32_t image; // 0x02
        uint8_t pad_06[0x12 - 0x06];

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
