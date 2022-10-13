#pragma once

#include "../Core/Span.hpp"
#include "Colour.h"
#include "ImageId.h"
#include <array>
#include <cstdint>
#include <optional>

namespace OpenLoco::Gfx::PaletteMap
{
    /**
     * The typical palette map size used to render images.
     */
    constexpr size_t kDefaultSize = 256;

    /**
     * Represents an 8-bit indexed map that maps from one palette index to another.
     */
    template<std::size_t TSize>
    using Buffer = std::array<PaletteIndex_t, TSize>;

    using View = stdx::span<const PaletteIndex_t>;

    View getDefault();

    std::optional<View> getForColour(ExtColour paletteId);
    std::optional<View> getFromImage(const ImageId image);
}
