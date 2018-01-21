#pragma once

#include "../localisation/stringmgr.h"
#include "../openloco.h"
#include <cstdint>

#define LOCO_G1_ELEMENT_COUNT 0x1A10

namespace openloco::gfx
{
#pragma pack(push, 1)

    struct drawpixelinfo_t
    {
        uint8_t* bits;       // 0x00
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t width;       // 0x08
        int16_t height;      // 0x0A
        int16_t pitch;       // 0x0C note: this is actually (pitch - width)
        uint16_t zoom_level; // 0x0E
    };

#pragma pack(pop)

    drawpixelinfo_t& screen_dpi();

    struct loco_g1_header
    {
        uint32_t num_entries;
        uint32_t total_size;
    };

     struct loco_g1_element
    {
        uint8_t * offset;       // 0x00
        int16_t width;          // 0x04
        int16_t height;         // 0x06
        int16_t x_offset;       // 0x08
        int16_t y_offset;       // 0x0A
        uint16_t flags;         // 0x0C
        int16_t unused;         // 0x0E
    };

    void load_g1();
    void clear(drawpixelinfo_t& dpi, uint32_t fill);
    void clear_single(drawpixelinfo_t& dpi, uint8_t paletteId);
    void draw_string_494B3F(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void draw_string_494BBF(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void invalidate_screen();
    void set_dirty_blocks(int32_t left, int32_t top, int32_t right, int32_t bottom);
}
