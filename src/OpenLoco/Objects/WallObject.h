#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    struct wall_object
    {
        string_id name;
        uint32_t sprite; // 0x02
        uint8_t var_06;
        uint8_t flags; // 0x07
        uint8_t var_08;
        uint8_t var_09;

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y);
    };
#pragma pack(pop)
}
