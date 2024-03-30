#pragma once

#include "DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>
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

        void drawString(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            const char* str) override;

        void drawStringLeft(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringLeftClipped(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringLeftUnderline(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        int16_t drawStringLeftWrapped(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringCentred(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringCentredClipped(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringCentredRaw(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            uint16_t linebreakCount,
            AdvancedColour colour,
            const char* wrappedStr) override;

        uint16_t drawStringCentredWrapped(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringRight(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            const void* args = nullptr) override;

        void drawStringRightUnderline(
            Gfx::RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            const void* args) override;

        void drawStringYOffsets(Gfx::RenderTarget& rt, Ui::Point loc, AdvancedColour colour, const void* args, const int8_t* yOffsets) override;
        void drawStringTicker(Gfx::RenderTarget& rt, Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width) override;
        uint16_t getStringWidthNewLined(const char* buffer) override;
        std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth) override;

        void fillRect(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags) override;
        void drawRect(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags) override;
        void fillRectInset(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags) override;
        void drawRectInset(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags) override;
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
