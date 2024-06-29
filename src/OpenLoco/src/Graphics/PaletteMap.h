#pragma once

#include "Colour.h"
#include "ImageId.h"
#include <array>
#include <cstdint>
#include <optional>
#include <span>

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

    using View = std::span<const PaletteIndex_t>;

    View getDefault();

    std::optional<View> getForColour(ExtColour paletteId);
    std::optional<View> getForImage(const ImageId image);

    void setEntryImage(ExtColour paletteId, uint32_t imageId);
}
