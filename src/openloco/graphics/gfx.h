#pragma once

#include <cstdint>
#include "../interop/interop.hpp"

namespace openloco::gfx
{
    #pragma pack(push, 1)

    struct drawpixelinfo_t
    {
        uint8_t * bits;         // 0x00
        int16_t x;              // 0x04
        int16_t y;              // 0x06
        int16_t width;          // 0x08
        int16_t height;         // 0x0A
        int16_t pitch;          // 0x0C note: this is actually (pitch - width)
        uint16_t zoom_level;    // 0x0E
    };

    #pragma pack(pop)

    void clear(drawpixelinfo_t &dpi, uint32_t fill);
    void invalidate_screen();
    void set_dirty_blocks(int32_t left, int32_t top, int32_t right, int32_t bottom);

    extern loco_global<drawpixelinfo_t, 0x0050B884> screen_dpi;
}
