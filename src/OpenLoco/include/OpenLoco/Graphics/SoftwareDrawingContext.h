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

        void reset();
        void pushRenderTarget(const RenderTarget& rt) override;
        void popRenderTarget() override;
        const RenderTarget& currentRenderTarget() const override;
        void clear(uint32_t fill) override;
        void clearSingle(uint8_t paletteId) override;
        void fillRect(int32_t left, int32_t top, int32_t right, int32_t bottom, uint8_t colour, RectFlags flags) override;
        void drawRect(int32_t x, int32_t y, int32_t dx, int32_t dy, uint8_t colour, RectFlags flags) override;
        void fillRectInset(int32_t left, int32_t top, int32_t right, int32_t bottom, AdvancedColour colour, RectInsetFlags flags) override;
        void drawRectInset(int32_t x, int32_t y, int32_t dx, int32_t dy, AdvancedColour colour, RectInsetFlags flags) override;
        void drawLine(const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour) override;
        void drawCircle(const Ui::Point& centre, int32_t radius, int32_t lineWidth, PaletteIndex_t colour) override;
        void drawImage(int32_t x, int32_t y, uint32_t image) override;
        void drawImage(const Ui::Point& pos, const ImageId& image) override;
        void drawImageMasked(const Ui::Point& pos, const ImageId& image, const ImageId& maskImage) override;
        void drawImageSolid(const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex) override;
        void drawImagePaletteSet(const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage) override;
    };
}
