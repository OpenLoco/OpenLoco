#include "PaletteMap.h"
#include "../Drawing/SoftwareDrawingEngine.h"
#include "../Interop/Interop.hpp"
#include "Gfx.h"
#include "ImageIds.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

using namespace OpenLoco::Interop;

namespace OpenLoco::Gfx::PaletteMap
{
    static loco_global<uint32_t[enumValue(ExtColour::max)], 0x050B8C8> _paletteToG1Offset;

    // Default immutable palette map.
    static const auto _defaultPaletteMapBuffer = [] {
        Buffer<kDefaultSize> data;
        std::iota(data.begin(), data.end(), 0);
        return data;
    }();

    // This buffer is used when sprites are drawn with a secondary palette.
    // TODO: Make this thread safe via thread_local if multi-threading is implemented.
    static auto _secondaryPaletteMapBuffer = _defaultPaletteMapBuffer;

    View getDefault()
    {
        return _defaultPaletteMapBuffer;
    }

    static void copyPaletteMapData(Buffer<kDefaultSize>& dst, size_t dstIndex, const View src, size_t srcIndex, size_t length)
    {
        auto maxLength = std::min(dst.size() - srcIndex, dst.size() - dstIndex);
        assert(length <= maxLength);
        auto copyLength = std::min(length, maxLength);
        std::copy_n(src.begin() + srcIndex, copyLength, dst.begin() + dstIndex);
    }

    static std::optional<uint32_t> getPaletteG1Index(ExtColour paletteId)
    {
        if (enumValue(paletteId) < std::size(_paletteToG1Offset))
        {
            return _paletteToG1Offset[enumValue(paletteId)];
        }
        return std::nullopt;
    }

    std::optional<View> getForColour(ExtColour paletteId)
    {
        auto g1Index = getPaletteG1Index(paletteId);
        if (g1Index)
        {
            auto g1 = getG1Element(*g1Index);
            if (g1 != nullptr)
            {
                const size_t length = g1->width * g1->height;

                // Palette maps must be of 256 entries per row.
                assert((length % kDefaultSize) == 0);

                return View(stdx::span{ g1->offset, length });
            }
        }
        return std::nullopt;
    }

    std::optional<View> getForImage(const ImageId image)
    {
        // No remapping required so use default palette map
        if (!image.hasPrimary() && !image.isBlended())
        {
            return std::nullopt; // Will use default
        }

        if (image.hasSecondary())
        {
            // Combines portions of two different palettes into the global palette map.
            auto& paletteMap = _secondaryPaletteMapBuffer;
            const auto primaryMap = getForColour(Colours::toExt(image.getPrimary()));
            const auto secondaryMap = getForColour(Colours::toExt(image.getSecondary()));
            if (!primaryMap || !secondaryMap)
            {
                assert(false);
            }

            // Remap sections are split into two bits for primary
            copyPaletteMapData(paletteMap, PaletteIndex::primaryRemap0, *primaryMap, PaletteIndex::primaryRemap0, (PaletteIndex::primaryRemap2 - PaletteIndex::primaryRemap0 + 1));
            copyPaletteMapData(paletteMap, PaletteIndex::primaryRemap3, *primaryMap, PaletteIndex::primaryRemap3, (PaletteIndex::primaryRemapB - PaletteIndex::primaryRemap3 + 1));
            copyPaletteMapData(paletteMap, PaletteIndex::secondaryRemap0, *secondaryMap, PaletteIndex::primaryRemap0, (PaletteIndex::primaryRemap2 - PaletteIndex::primaryRemap0 + 1));
            copyPaletteMapData(paletteMap, PaletteIndex::secondaryRemap3, *secondaryMap, PaletteIndex::primaryRemap3, (PaletteIndex::primaryRemapB - PaletteIndex::primaryRemap3 + 1));

            return paletteMap;
        }
        else
        {
            if (image.isBlended())
            {
                return getForColour(image.getTranslucency());
            }
            else
            {
                // For primary flagged images
                return getForColour(image.getRemap());
            }
        }
    }

}
