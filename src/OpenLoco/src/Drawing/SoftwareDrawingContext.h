#pragma once

#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include "Ui/Rect.h"
#include <cstdint>

namespace OpenLoco::Drawing
{
    class SoftwareDrawingContext
    {
    public:
        void clear(Gfx::RenderTarget& rt, uint32_t fill);
        void clearSingle(Gfx::RenderTarget& rt, uint8_t paletteId);

        int16_t clipString(int16_t width, char* string);
        uint16_t getStringWidth(const char* buffer);
        uint16_t getMaxStringWidth(const char* buffer);

        Ui::Point drawString(Gfx::RenderTarget& rt, int16_t x, int16_t y, AdvancedColour colour, void* str);

        int16_t drawStringLeftWrapped(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringLeft(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringLeft(
            Gfx::RenderTarget& rt,
            Ui::Point* origin,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringLeftClipped(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringRight(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringRightUnderline(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args);
        void drawStringLeftUnderline(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringCentred(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringCentredClipped(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        uint16_t drawStringCentredWrapped(
            Gfx::RenderTarget& rt,
            Ui::Point& origin,
            uint16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr);
        void drawStringCentredRaw(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            const void* args);
        void drawStringYOffsets(Gfx::RenderTarget& rt, const Ui::Point& loc, AdvancedColour colour, const void* args, const int8_t* yOffsets);
        uint16_t getStringWidthNewLined(const char* buffer);
        std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth);

        void fillRect(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
        void drawRect(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour);
        void fillRectInset(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags);
        void drawRectInset(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags);
        void drawLine(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
        void drawImage(Gfx::RenderTarget* rt, int16_t x, int16_t y, uint32_t image);
        void drawImage(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image);
        void drawImageSolid(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex);
        void drawImagePaletteSet(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, Gfx::PaletteMap::View palette, const Gfx::G1Element* noiseImage);

        // 0x0112C876
        int16_t getCurrentFontSpriteBase();
        void setCurrentFontSpriteBase(int16_t base);
    };
}
