#pragma once

#include "DrawingContext.h"
#include "Font.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>

namespace OpenLoco::Gfx
{
    class SoftwareDrawingContext final : public DrawingContext
    {
    public:
        void clear(const RenderTarget& rt, uint32_t fill) override;
        void clearSingle(const RenderTarget& rt, uint8_t paletteId) override;
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

    public:
        // Legacy text drawing functions, use TextRenderer instead.

        int16_t clipString(int16_t width, char* string) override;
        uint16_t getStringWidth(const char* buffer) override;
        uint16_t getMaxStringWidth(const char* buffer) override;

        void drawStringYOffsets(const RenderTarget& rt, Ui::Point loc, AdvancedColour colour, const char* str, const int8_t* yOffsets) override;
        void drawStringTicker(const RenderTarget& rt, Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width) override;
        uint16_t getStringWidthNewLined(const char* buffer) override;
        std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth) override;

        // 0x0112C876
        Font getCurrentFont() override;
        void setCurrentFont(Font base) override;
    };
}
