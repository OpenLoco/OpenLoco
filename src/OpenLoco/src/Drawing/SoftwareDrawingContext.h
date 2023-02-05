#pragma once

#include "DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include "Ui/Rect.h"
#include <cstdint>

namespace OpenLoco::Drawing
{
    class SoftwareDrawingContext final : DrawingContext
    {
    public:
        void clear(Gfx::RenderTarget& rt, uint32_t fill) override;
        void clearSingle(Gfx::RenderTarget& rt, uint8_t paletteId) override;

        int16_t clipString(int16_t width, char* string) override;
        uint16_t getStringWidth(const char* buffer) override;
        uint16_t getMaxStringWidth(const char* buffer) override;

        Ui::Point drawString(Gfx::RenderTarget& rt, int16_t x, int16_t y, AdvancedColour colour, void* str) override;

        int16_t drawStringLeftWrapped(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringLeft(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringLeft(
            Gfx::RenderTarget& rt,
            Ui::Point* origin,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringLeftClipped(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringRight(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringRightUnderline(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args) override;
        void drawStringLeftUnderline(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringCentred(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringCentredClipped(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        uint16_t drawStringCentredWrapped(
            Gfx::RenderTarget& rt,
            Ui::Point& origin,
            uint16_t width,
            AdvancedColour colour,
            string_id stringId,
            const void* args = nullptr) override;
        void drawStringCentredRaw(
            Gfx::RenderTarget& rt,
            int16_t x,
            int16_t y,
            int16_t width,
            AdvancedColour colour,
            const void* args) override;
        void drawStringYOffsets(Gfx::RenderTarget& rt, const Ui::Point& loc, AdvancedColour colour, const void* args, const int8_t* yOffsets) override;
        uint16_t getStringWidthNewLined(const char* buffer) override;
        std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth) override;

        void fillRect(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour) override;
        void drawRect(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour) override;
        void fillRectInset(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags) override;
        void drawRectInset(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags) override;
        void drawLine(Gfx::RenderTarget& rt, const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour) override;
        void drawImage(Gfx::RenderTarget* rt, int16_t x, int16_t y, uint32_t image) override;
        void drawImage(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image) override;
        void drawImageMasked(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage) override;
        void drawImageSolid(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex) override;
        void drawImagePaletteSet(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, Gfx::PaletteMap::View palette, const Gfx::G1Element* noiseImage) override;

        // 0x0112C876
        int16_t getCurrentFontSpriteBase() override;
        void setCurrentFontSpriteBase(int16_t base) override;
    };
}
