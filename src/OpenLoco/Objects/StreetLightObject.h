#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    struct StreetLightObject
    {
        string_id name;
        uint16_t designedYear[3]; // 0x02
        uint32_t image;           // 0x08

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
