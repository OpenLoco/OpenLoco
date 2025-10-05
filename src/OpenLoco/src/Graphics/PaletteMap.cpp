#include "PaletteMap.h"
#include "Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ImageIds.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

using namespace OpenLoco::Interop;

namespace OpenLoco::Gfx::PaletteMap
{
    // 0x0050B8C8
    // Maps ExtColour enum values to G1 image indices for palette map data
    static std::array<uint32_t, enumValue(ExtColour::max)> _paletteToG1Offset = { {
        // clang-format off
        2170, 2171, 2172, 2173, 2174, 2175, 2176, 2177, // 0x00-0x07: black-blue
        2178, 2179, 2180, 2181, 2182, 2183, 2184, 2185, // 0x08-0x0F: mutedDarkTeal-mutedOliveGreen
        2186, 2187, 2188, 2189, 2190, 2191, 2192, 2193, // 0x10-0x17: yellow-brown
        2194, 2195, 2196, 2197, 2198, 2199, 2200, 2201, // 0x18-0x1F: mutedOrange-mutedRed, clear
        0,                                               // 0x20: water (dynamically loaded)
        417, 418, 419, 420, 421, 423, 424, 425,         // 0x21-0x28: unk21-unk28
        426, 427, 422,                                   // 0x29-0x2B: unk29-unk2B
        2202, 2203, 2204, 2205, 2206, 2207, 2208, 2209, // 0x2C-0x33: unk2C-translucentGrey1
        2210, 2211, 2212, 2213, 2214, 2215, 2216, 2217, // 0x34-0x3B: translucentGrey2-translucentMutedSeaGreen0
        2218, 2219, 2220, 2221, 2222, 2223, 2224, 2225, // 0x3C-0x43: translucentMutedPurple1-translucentYellow0
        2226, 2227, 2228, 2229, 2230, 2231, 2232, 2233, // 0x44-0x4B: translucentMutedGrassGreen1-translucentGreen0
        2234, 2235, 2236, 2237, 2238, 2239, 2240, 2241, // 0x4C-0x53: translucentMutedOrange1-translucentRed0
        2242, 2243, 2244, 2245, 2246, 2247, 2248, 2249, // 0x54-0x5B: translucentOrange1-translucentPink0
        2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, // 0x5C-0x63: translucentBrown1-translucentAmber0
        2258, 2259, 2260, 2261, 2262, 2263, 2264, 2265, // 0x64-0x6B: unk74-unk7B
        2266, 2267, 2268, 2269, 2270, 2271, 2272, 2273, // 0x6C-0x73: unk7C-unk83
        2274, 2275, 2276, 2277, 2278, 2279, 2280, 2281, // 0x74-0x7B: unk84-unk8B
        2282, 2283, 2284, 2285, 2286, 2287, 2288, 2289, // 0x7C-0x83: unk8C-unk93
        2290, 2291, 2292, 2293, 2294, 2295, 2296, 2297, // 0x84-0x8B: (continued)
        2298, 2299, 2300, 2301, 2302, 2303, 2304,       // 0x8C-0x92: (continued to max)
        // clang-format on
    } };

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

                return View(std::span{ g1->offset, length });
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

    void setEntryImage(ExtColour paletteId, uint32_t imageId)
    {
        assert(enumValue(paletteId) < std::size(_paletteToG1Offset));

        _paletteToG1Offset[enumValue(paletteId)] = imageId;
    }

}
