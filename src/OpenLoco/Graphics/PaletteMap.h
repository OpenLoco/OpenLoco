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
    struct PaletteMap
    {
    private:
        stdx::span<uint8_t> _data{};

    public:
        static PaletteMap getDefault();

        constexpr PaletteMap() = default;

        constexpr PaletteMap(stdx::span<uint8_t> data) noexcept
            : _data(data)
        {
        }

        uint8_t operator[](size_t index) const;
        uint8_t* data() const { return _data.data(); }
        uint8_t blend(uint8_t src, uint8_t dst) const;
        void copy(size_t dstIndex, const PaletteMap& src, size_t srcIndex, size_t length);
    };

    std::optional<uint32_t> getPaletteG1Index(ExtColour paletteId);
    std::optional<PaletteMap> getPaletteMapForColour(ExtColour paletteId);
    std::optional<PaletteMap> getPaletteMapFromImage(const ImageId image);
}
