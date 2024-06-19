#pragma once

#include "Font.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Localisation/FormatArguments.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>

namespace OpenLoco::Gfx
{
    enum class RectInsetFlags : uint8_t
    {
        fillTransparent = 1U << 2, // ? unused
        borderNone = 1U << 3,      // ? unused
        fillNone = 1U << 4,
        borderInset = 1U << 5,
        fillDarker = 1U << 6,
        colourLight = 1U << 7,
        none = 0U
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RectInsetFlags);

    enum class RectFlags : uint32_t
    {
        crossHatching = 1U << 24,
        transparent = 1U << 25,   // Changes colour parameter from PaletteIndex_t to ExtColour
        selectPattern = 1U << 26, // unused
        g1Pattern = 1U << 27,     // unused
        none = 0U
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RectFlags);

    class DrawingContext
    {
    public:
        virtual ~DrawingContext() = default;

        virtual void pushRenderTarget(const RenderTarget& rt) = 0;

        virtual void popRenderTarget() = 0;

        virtual const RenderTarget& currentRenderTarget() const = 0;

        virtual void clear(uint32_t fill) = 0;

        virtual void clearSingle(uint8_t paletteId) = 0;

        virtual void fillRect(int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags) = 0;

        virtual void drawRect(int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags) = 0;

        virtual void fillRectInset(int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags) = 0;

        virtual void drawRectInset(int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags) = 0;

        virtual void drawLine(const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour) = 0;

        virtual void drawImage(int16_t x, int16_t y, uint32_t image) = 0;

        virtual void drawImage(const Ui::Point& pos, const ImageId& image) = 0;

        virtual void drawImageMasked(const Ui::Point& pos, const ImageId& image, const ImageId& maskImage) = 0;

        virtual void drawImageSolid(const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex) = 0;

        virtual void drawImagePaletteSet(const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage) = 0;
    };
}
