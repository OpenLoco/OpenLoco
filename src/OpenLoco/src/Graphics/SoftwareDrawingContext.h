#pragma once

#include "DrawingContext.h"
#include "Font.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>
#include <memory>

namespace OpenLoco::Gfx
{
    struct SoftwareDrawingContextState;

    class SoftwareDrawingContext final : public DrawingContext
    {
    private:
        std::unique_ptr<SoftwareDrawingContextState> _state;

    public:
        SoftwareDrawingContext();
        ~SoftwareDrawingContext() override;

        void pushRenderTarget(const RenderTarget& rt) override;
        void popRenderTarget() override;
        const RenderTarget& currentRenderTarget() const override;
        void clear(uint32_t fill) override;
        void clearSingle(uint8_t paletteId) override;
        void fillRect(int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags) override;
        void drawRect(int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags) override;
        void fillRectInset(int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags) override;
        void drawRectInset(int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags) override;
        void drawLine(const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour) override;
        void drawImage(int16_t x, int16_t y, uint32_t image) override;
        void drawImage(const Ui::Point& pos, const ImageId& image) override;
        void drawImageMasked(const Ui::Point& pos, const ImageId& image, const ImageId& maskImage) override;
        void drawImageSolid(const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex) override;
        void drawImagePaletteSet(const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage) override;
    };
}
