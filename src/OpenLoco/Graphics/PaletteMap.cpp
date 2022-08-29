#include "PaletteMap.h"
#include "../Drawing/SoftwareDrawingEngine.h"
#include "../Interop/Interop.hpp"
#include "Gfx.h"
#include "ImageIds.h"
#include <algorithm>
#include <cassert>
#include <numeric>

using namespace OpenLoco::Interop;

namespace OpenLoco::Gfx
{
    static loco_global<uint32_t[147], 0x050B8C8> _paletteToG1Offset;

    const PaletteMap& PaletteMap::getDefault()
    {
        static bool initialised = false;
        static uint8_t data[256];
        static PaletteMap defaultMap(data);
        if (!initialised)
        {
            std::iota(std::begin(data), std::end(data), 0);
            initialised = true;
        }
        return defaultMap;
    }

    uint8_t& PaletteMap::operator[](size_t index)
    {
        assert(index < _dataLength);

        // Provide safety in release builds
        if (index >= _dataLength)
        {
            static uint8_t dummy;
            return dummy;
        }

        return _data[index];
    }

    uint8_t PaletteMap::operator[](size_t index) const
    {
        assert(index < _dataLength);

        // Provide safety in release builds
        if (index >= _dataLength)
        {
            return 0;
        }

        return _data[index];
    }

    uint8_t PaletteMap::blend(uint8_t src, uint8_t dst) const
    {
        // src = 0 would be transparent so there is no blend palette for that, hence (src - 1)
        assert(src != 0 && (src - 1) < _numMaps);
        assert(dst < _mapLength);
        auto idx = ((src - 1) * 256) + dst;
        return (*this)[idx];
    }

    void PaletteMap::copy(size_t dstIndex, const PaletteMap& src, size_t srcIndex, size_t length)
    {
        auto maxLength = std::min(_mapLength - srcIndex, _mapLength - dstIndex);
        assert(length <= maxLength);
        auto copyLength = std::min(length, maxLength);
        std::memcpy(&_data[dstIndex], &src._data[srcIndex], copyLength);
    }

    std::optional<uint32_t> getPaletteG1Index(ExtColour paletteId)
    {
        if (enumValue(paletteId) < std::size(_paletteToG1Offset))
        {
            return _paletteToG1Offset[enumValue(paletteId)];
        }
        return std::nullopt;
    }

    std::optional<PaletteMap> getPaletteMapForColour(ExtColour paletteId)
    {
        auto g1Index = getPaletteG1Index(paletteId);
        if (g1Index)
        {
            auto g1 = getG1Element(*g1Index);
            if (g1 != nullptr)
            {
                return PaletteMap(g1->offset, g1->height, g1->width);
            }
        }
        return std::nullopt;
    }

    std::optional<PaletteMap> getPaletteMapFromImage(const ImageId image)
    {
        // No remapping required so use default palette map
        if (!image.hasPrimary() && !image.isBlended())
        {
            return std::nullopt; // Will use default
        }

        if (image.hasSecondary())
        {
            // A secondary paletteMap is made up by combinging bits from two palettes.
            PaletteMap customMap = PaletteMap::getDefault();
            const auto primaryMap = getPaletteMapForColour(Colours::toExt(image.getPrimary()));
            const auto secondaryMap = getPaletteMapForColour(Colours::toExt(image.getSecondary()));
            if (!primaryMap || !secondaryMap)
            {
                assert(false);
            }
            // Remap sections are split into two bits for primary
            customMap.copy(PaletteIndex::primaryRemap0, *primaryMap, PaletteIndex::primaryRemap0, (PaletteIndex::primaryRemap2 - PaletteIndex::primaryRemap0 + 1));
            customMap.copy(PaletteIndex::primaryRemap3, *primaryMap, PaletteIndex::primaryRemap3, (PaletteIndex::primaryRemapB - PaletteIndex::primaryRemap3 + 1));
            customMap.copy(PaletteIndex::secondaryRemap0, *secondaryMap, PaletteIndex::primaryRemap0, (PaletteIndex::primaryRemap2 - PaletteIndex::primaryRemap0 + 1));
            customMap.copy(PaletteIndex::secondaryRemap3, *secondaryMap, PaletteIndex::primaryRemap3, (PaletteIndex::primaryRemapB - PaletteIndex::primaryRemap3 + 1));

            // TODO: Investigate if this can be simplified by just copying the primary map in full to begin with
            // then it would only need to fill in the secondary remap section
            return customMap;
        }
        else
        {
            if (image.isBlended())
            {
                return getPaletteMapForColour(image.getTranslucency());
            }
            else
            {
                // For primary flagged images
                return getPaletteMapForColour(image.getRemap());
            }
        }
    }
}
