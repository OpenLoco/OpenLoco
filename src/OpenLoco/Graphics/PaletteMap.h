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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
        uint16_t _numMaps;
#pragma clang diagnostic pop
        uint16_t _mapLength;

    public:
        static const PaletteMap& getDefault();

        PaletteMap() = default;

        PaletteMap(uint8_t* data, uint16_t numMaps, uint16_t mapLength)
            : _data{data, numMaps * mapLength}
            , _numMaps(numMaps)
            , _mapLength(mapLength)
        {
        }

        constexpr PaletteMap(stdx::span<uint8_t> data)
            : _data(data)
            , _numMaps(1)
            , _mapLength(static_cast<uint16_t>(data.size()))
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
