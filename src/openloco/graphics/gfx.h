#pragma once

#include "../localisation/stringmgr.h"
#include "../openloco.h"
#include <cstdint>

namespace openloco::gfx
{
#pragma pack(push, 1)

    struct GraphicsContext
    {
        uint8_t* bits;       // 0x00
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t width;       // 0x08
        int16_t height;      // 0x0A
        int16_t pitch;       // 0x0C note: this is actually (pitch - width)
        uint16_t zoom_level; // 0x0E
    };

    GraphicsContext& getScreenGraphicsContext();

    struct g1_header_t
    {
        uint32_t num_entries;
        uint32_t total_size;
    };

    struct g1_element32_t
    {
        uint32_t offset;  // 0x00
        int16_t width;    // 0x04
        int16_t height;   // 0x06
        int16_t x_offset; // 0x08
        int16_t y_offset; // 0x0A
        uint16_t flags;   // 0x0C
        int16_t unused;   // 0x0E
    };

    // A version that can be 64-bit when ready...
    struct g1_element
    {
        uint8_t* offset = nullptr;
        int16_t width = 0;
        int16_t height = 0;
        int16_t x_offset = 0;
        int16_t y_offset = 0;
        uint16_t flags = 0;
        int16_t unused = 0;

        g1_element() = default;
        g1_element(const g1_element32_t& src)
            : offset((uint8_t*)src.offset)
            , width(src.width)
            , height(src.height)
            , x_offset(src.x_offset)
            , y_offset(src.y_offset)
            , flags(src.flags)
            , unused(src.unused)
        {
        }
    };

#pragma pack(pop)

    struct point_t
    {
        int16_t x;
        int16_t y;
    };

    struct ui_size_t
    {
        uint16_t width;
        uint16_t height;
    };

    GraphicsContext& getScreenGraphicsContext();

    void load_g1();
    void clear(GraphicsContext& context, uint32_t fill);
    void clear_single(GraphicsContext& context, uint8_t paletteId);

    int16_t clip_string(int16_t width, char* string);
    uint16_t get_string_width(const char* buffer);

    void draw_string(
        GraphicsContext& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        const char* string);

    void draw_string_494B3F(
        GraphicsContext& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void draw_string_494B3F(
        GraphicsContext& context,
        point_t* origin,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void draw_string_494BBF(
        GraphicsContext& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void draw_string_centred(
        GraphicsContext& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const char* args);
    void draw_string_centred_clipped(
        GraphicsContext& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const char* args);
    void draw_string_centred_wrapped(
        GraphicsContext* context,
        point_t* origin,
        uint16_t width,
        uint8_t colour,
        string_id stringId,
        const char* args = nullptr);

    void fill_rect(gfx::GraphicsContext* context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t color);
    void draw_rect(gfx::GraphicsContext* context, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t color);
    void fill_rect_inset(gfx::GraphicsContext* context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t color, uint8_t flags);
    void draw_image(gfx::GraphicsContext* context, int16_t x, int16_t y, uint32_t image);
    void draw_image_solid(gfx::GraphicsContext* context, int16_t x, int16_t y, uint32_t image, uint8_t palette_index);
    void invalidate_screen();
    void set_dirty_blocks(int32_t left, int32_t top, int32_t right, int32_t bottom);

    bool clipGraphicsContext(gfx::GraphicsContext** dst, gfx::GraphicsContext* src, int16_t x, int16_t y, int16_t width, int16_t height);
}
