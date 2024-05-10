#pragma once

#include "DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>

namespace OpenLoco::Gfx
{
    class SoftwareDrawingContext final : DrawingContext
    {
    public:
        void clear(const RenderTarget& rt, uint32_t fill) override;
        void clearSingle(const RenderTarget& rt, uint8_t paletteId) override;

        int16_t clipString(int16_t width, char* string) override;
        uint16_t getStringWidth(const char* buffer) override;
        uint16_t getMaxStringWidth(const char* buffer) override;

        Ui::Point drawString(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            const char* str) override;

        Ui::Point drawStringLeft(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringLeftClipped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringLeftUnderline(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringLeftWrapped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringCentred(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringCentredClipped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringCentredRaw(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t linebreakCount,
            AdvancedColour colour,
            const char* wrappedStr) override;

        Ui::Point drawStringCentredWrapped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringRight(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        Ui::Point drawStringRightUnderline(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {}) override;

        void drawStringYOffsets(const RenderTarget& rt, Ui::Point loc, AdvancedColour colour, const void* args, const int8_t* yOffsets) override;
        void drawStringTicker(const RenderTarget& rt, Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width) override;
        uint16_t getStringWidthNewLined(const char* buffer) override;
        std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth) override;

        void fillRect(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags) override;
        void drawRect(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags) override;
        void fillRectInset(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags) override;
        void drawRectInset(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags) override;
        void drawLine(const RenderTarget& rt, const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour) override;
        void drawImage(const RenderTarget* rt, int16_t x, int16_t y, uint32_t image) override;
        void drawImage(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image) override;
        void drawImageMasked(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage) override;
        void drawImageSolid(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex) override;
        void drawImagePaletteSet(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage) override;

        // 0x0112C876
        int16_t getCurrentFontSpriteBase() override;
        void setCurrentFontSpriteBase(int16_t base) override;
    };
}
