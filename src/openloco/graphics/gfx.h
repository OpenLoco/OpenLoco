#pragma once

#include "../OpenLoco.h"
#include "../types.hpp"
#include "../ui/Rect.h"
#include "types.h"
#include <cstdint>

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

    drawpixelinfo_t& screenDpi();

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

    drawpixelinfo_t& screenDpi();

    void loadG1();
    void clear(drawpixelinfo_t& dpi, uint32_t fill);
    void clearSingle(drawpixelinfo_t& dpi, uint8_t paletteId);

    int16_t clipString(int16_t width, char* string);
    uint16_t getStringWidth(const char* buffer);

    gfx::point_t drawString(drawpixelinfo_t* context, int16_t x, int16_t y, uint8_t colour, void* str);

    int16_t drawString_495224(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494B3F(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494B3F(
        drawpixelinfo_t& dpi,
        point_t* origin,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494BBF(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494C78(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringUnderline(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void drawStringLeftUnderline(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentred(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredClipped(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredWrapped(
        drawpixelinfo_t* context,
        point_t* origin,
        uint16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredRaw(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        const void* args);
    uint16_t getStringWidthNewLined(const char* buffer);
    std::pair<uint16_t, uint16_t> wrapString(const char* buffer, uint16_t stringWidth);

    void fillRect(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
    void drawRect(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour);
    void fillRectInset(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags);
    void drawRectInset(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags);
    void drawLine(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
    void drawImage(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image);
    void drawImageSolid(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image, uint8_t palette_index);
    void drawImagePaletteSet(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image, uint8_t* palette);
    uint32_t recolour(uint32_t image);
    uint32_t recolour(uint32_t image, uint8_t colour);

    void invalidateScreen();
    void setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void drawDirtyBlocks();
    void render();

    void redrawScreenRect(ui::Rect rect);
    void redrawScreenRect(int16_t left, int16_t top, int16_t right, int16_t bottom);

    bool clipDrawpixelinfo(gfx::drawpixelinfo_t** dst, gfx::drawpixelinfo_t* src, int16_t x, int16_t y, int16_t width, int16_t height);
    g1_element* getG1Element(uint32_t id);
}
