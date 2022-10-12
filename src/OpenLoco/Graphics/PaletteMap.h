#pragma once

#include "../Core/Span.hpp"
#include "Colour.h"
#include "ImageId.h"
#include <array>
#include <cstdint>
#include <optional>

namespace OpenLoco::Gfx
{
    /**
     * Represents an 8-bit indexed map that maps from one palette index to another.
     */
    using PaletteMapBuffer = std::array<uint8_t, 256>;
    using PaletteMapView = stdx::span<const uint8_t>;

    PaletteMapView getDefaultPaletteMap();

    std::optional<uint32_t> getPaletteG1Index(ExtColour paletteId);
    std::optional<PaletteMapView> getPaletteMapForColour(ExtColour paletteId);
    std::optional<PaletteMapView> getPaletteMapFromImage(const ImageId image);
}
