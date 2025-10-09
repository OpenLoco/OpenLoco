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
    static constexpr std::array<uint32_t, enumValue(ExtColour::max)> kDefaultPaletteToG1Offset = {
        /* 2170, //*/ ImageIds::paletteMapBlack,                         // black
        /* 2171, //*/ ImageIds::paletteMapGrey,                          // grey
        /* 2172, //*/ ImageIds::paletteMapWhite,                         // white
        /* 2173, //*/ ImageIds::paletteMapMutedDarkPurple,               // mutedDarkPurple
        /* 2174, //*/ ImageIds::paletteMapMutedPurple,                   // mutedPurple
        /* 2175, //*/ ImageIds::paletteMapPurple,                        // purple
        /* 2176, //*/ ImageIds::paletteMapDarkBlue,                      // darkBlue
        /* 2177, //*/ ImageIds::paletteMapBlue,                          // blue
        /* 2178, //*/ ImageIds::paletteMapMutedDarkTeal,                 // mutedDarkTeal
        /* 2179, //*/ ImageIds::paletteMapMutedTeal,                     // mutedTeal
        /* 2180, //*/ ImageIds::paletteMapDarkGreen,                     // darkGreen
        /* 2181, //*/ ImageIds::paletteMapMutedSeaGreen,                 // mutedSeaGreen
        /* 2182, //*/ ImageIds::paletteMapMutedGrassGreen,               // mutedGrassGreen
        /* 2183, //*/ ImageIds::paletteMapGreen,                         // green
        /* 2184, //*/ ImageIds::paletteMapMutedAvocadoGreen,             // mutedAvocadoGreen
        /* 2185, //*/ ImageIds::paletteMapMutedOliveGreen,               // mutedOliveGreen
        /* 2186, //*/ ImageIds::paletteMapYellow,                        // yellow
        /* 2187, //*/ ImageIds::paletteMapDarkYellow,                    // darkYellow
        /* 2188, //*/ ImageIds::paletteMapOrange,                        // orange
        /* 2189, //*/ ImageIds::paletteMapAmber,                         // amber
        /* 2190, //*/ ImageIds::paletteMapDarkOrange,                    // darkOrange
        /* 2191, //*/ ImageIds::paletteMapMutedDarkYellow,               // mutedDarkYellow
        /* 2192, //*/ ImageIds::paletteMapMutedYellow,                   // mutedYellow
        /* 2193, //*/ ImageIds::paletteMapBrown,                         // brown
        /* 2194, //*/ ImageIds::paletteMapMutedOrange,                   // mutedOrange
        /* 2195, //*/ ImageIds::paletteMapMutedDarkRed,                  // mutedDarkRed
        /* 2196, //*/ ImageIds::paletteMapDarkRed,                       // darkRed
        /* 2197, //*/ ImageIds::paletteMapRed,                           // red
        /* 2198, //*/ ImageIds::paletteMapDarkPink,                      // darkPink
        /* 2199, //*/ ImageIds::paletteMapPink,                          // pink
        /* 2200, //*/ ImageIds::paletteMapMutedRed,                      // mutedRed
        /* 2201, //*/ ImageIds::paletteMapClear,                         // clear
        /*0, //*/ 0,                                                 // water (no palette map will be loaded from water object)
        /* 417, //*/ ImageIds::paletteMapUnk21,                         // unk21
        /* 418, //*/ ImageIds::paletteMapUnk22,                         // unk22
        /* 419, //*/ ImageIds::paletteMapUnk23,                         // unk23
        /* 420, //*/ ImageIds::paletteMapUnk24,                         // unk24
        /* 421, //*/ ImageIds::paletteMapUnk25,                         // unk25
        /* 423, //*/ ImageIds::paletteMapUnk26,                         // unk26
        /* 424, //*/ ImageIds::paletteMapUnk27,                         // unk27
        /* 425, //*/ ImageIds::paletteMapUnk28,                         // unk28
        /* 426, //*/ ImageIds::paletteMapUnk29,                         // unk29
        /* 427, //*/ ImageIds::paletteMapUnk2A,                         // unk2A
        /* 422, //*/ ImageIds::paletteMapUnk2B,                         // unk2B
        /* 2202, //*/ ImageIds::paletteMapUnk2C,                         // unk2C
        /* 2203, //*/ ImageIds::paletteMapUnk2D,                         // unk2D
        /* 2204, //*/ ImageIds::paletteMapUnk2E,                         // unk2E
        /* 2205, //*/ ImageIds::paletteMapUnk2F,                         // unk2F
        /* 2206, //*/ ImageIds::paletteMapUnk30,                         // unk30
        /* 2207, //*/ ImageIds::paletteMapUnk31,                         // unk31
        /* 2208, //*/ ImageIds::paletteMapUnk32,                         // unk32
        /* 2209, //*/ ImageIds::paletteMapUnk33,                         // unk33
        /* 2210, //*/ ImageIds::paletteMapUnk34,                         // unk34
        /* 2211, //*/ ImageIds::paletteMapTranslucentGrey1,              // translucentGrey1
        /* 2212, //*/ ImageIds::paletteMapTranslucentGrey2,              // translucentGrey2
        /* 2213, //*/ ImageIds::paletteMapTranslucentGrey0,              // translucentGrey0
        /* 2214, //*/ ImageIds::paletteMapTranslucentBlue1,              // translucentBlue1
        /* 2215, //*/ ImageIds::paletteMapTranslucentBlue2,              // translucentBlue2
        /* 2216, //*/ ImageIds::paletteMapTranslucentBlue0,              // translucentBlue0
        /* 2217, //*/ ImageIds::paletteMapTranslucentMutedDarkRed1,      // translucentMutedDarkRed1
        /* 2218, //*/ ImageIds::paletteMapTranslucentMutedDarkRed2,      // translucentMutedDarkRed2
        /* 2219, //*/ ImageIds::paletteMapTranslucentMutedDarkRed0,      // translucentMutedDarkRed0
        /* 2220, //*/ ImageIds::paletteMapTranslucentMutedSeaGreen1,     // translucentMutedSeaGreen1
        /* 2221, //*/ ImageIds::paletteMapTranslucentMutedSeaGreen2,     // translucentMutedSeaGreen2
        /* 2222, //*/ ImageIds::paletteMapTranslucentMutedSeaGreen0,     // translucentMutedSeaGreen0
        /* 2223, //*/ ImageIds::paletteMapTranslucentMutedPurple1,       // translucentMutedPurple1
        /* 2224, //*/ ImageIds::paletteMapTranslucentMutedPurple2,       // translucentMutedPurple2
        /* 2225, //*/ ImageIds::paletteMapTranslucentMutedPurple0,       // translucentMutedPurple0
        /* 2226, //*/ ImageIds::paletteMapTranslucentMutedOliveGreen1,   // translucentMutedOliveGreen1
        /* 2227, //*/ ImageIds::paletteMapTranslucentMutedOliveGreen2,   // translucentMutedOliveGreen2
        /* 2228, //*/ ImageIds::paletteMapTranslucentMutedOliveGreen0,   // translucentMutedOliveGreen0
        /* 2229, //*/ ImageIds::paletteMapTranslucentMutedYellow1,       // translucentMutedYellow1
        /* 2230, //*/ ImageIds::paletteMapTranslucentMutedYellow2,       // translucentMutedYellow2
        /* 2231, //*/ ImageIds::paletteMapTranslucentMutedYellow0,       // translucentMutedYellow0
        /* 2232, //*/ ImageIds::paletteMapTranslucentYellow1,            // translucentYellow1
        /* 2233, //*/ ImageIds::paletteMapTranslucentYellow2,            // translucentYellow2
        /* 2234, //*/ ImageIds::paletteMapTranslucentYellow0,            // translucentYellow0
        /* 2235, //*/ ImageIds::paletteMapTranslucentMutedGrassGreen1,   // translucentMutedGrassGreen1
        /* 2236, //*/ ImageIds::paletteMapTranslucentMutedGrassGreen2,   // translucentMutedGrassGreen2
        /* 2237, //*/ ImageIds::paletteMapTranslucentMutedGrassGreen0,   // translucentMutedGrassGreen0
        /* 2238, //*/ ImageIds::paletteMapTranslucentMutedAvocadoGreen1, // translucentMutedAvocadoGreen1
        /* 2239, //*/ ImageIds::paletteMapTranslucentMutedAvocadoGreen2, // translucentMutedAvocadoGreen2
        /* 2240, //*/ ImageIds::paletteMapTranslucentMutedAvocadoGreen0, // translucentMutedAvocadoGreen0
        /* 2241, //*/ ImageIds::paletteMapTranslucentGreen1,             // translucentGreen1
        /* 2242, //*/ ImageIds::paletteMapTranslucentGreen2,             // translucentGreen2
        /* 2243, //*/ ImageIds::paletteMapTranslucentGreen0,             // translucentGreen0
        /* 2244, //*/ ImageIds::paletteMapTranslucentMutedOrange1,       // translucentMutedOrange1
        /* 2245, //*/ ImageIds::paletteMapTranslucentMutedOrange2,       // translucentMutedOrange2
        /* 2246, //*/ ImageIds::paletteMapTranslucentMutedOrange0,       // translucentMutedOrange0
        /* 2247, //*/ ImageIds::paletteMapTranslucentPurple1,            // translucentPurple1
        /* 2248, //*/ ImageIds::paletteMapTranslucentPurple2,            // translucentPurple2
        /* 2249, //*/ ImageIds::paletteMapTranslucentPurple0,            // translucentPurple0
        /* 2250, //*/ ImageIds::paletteMapTranslucentRed1,               // translucentRed1
        /* 2251, //*/ ImageIds::paletteMapTranslucentRed2,               // translucentRed2
        /* 2252, //*/ ImageIds::paletteMapTranslucentRed0,               // translucentRed0
        /* 2253, //*/ ImageIds::paletteMapTranslucentOrange1,            // translucentOrange1
        /* 2254, //*/ ImageIds::paletteMapTranslucentOrange2,            // translucentOrange2
        /* 2255, //*/ ImageIds::paletteMapTranslucentOrange0,            // translucentOrange0
        /* 2256, //*/ ImageIds::paletteMapTranslucentMutedTeal1,         // translucentMutedTeal1
        /* 2257, //*/ ImageIds::paletteMapTranslucentMutedTeal2,         // translucentMutedTeal2
        /* 2258, //*/ ImageIds::paletteMapTranslucentMutedTeal0,         // translucentMutedTeal0
        /* 2259, //*/ ImageIds::paletteMapTranslucentPink1,              // translucentPink1
        /* 2260, //*/ ImageIds::paletteMapTranslucentPink2,              // translucentPink2
        /* 2261, //*/ ImageIds::paletteMapTranslucentPink0,              // translucentPink0
        /* 2262, //*/ ImageIds::paletteMapTranslucentBrown1,             // translucentBrown1
        /* 2263, //*/ ImageIds::paletteMapTranslucentBrown2,             // translucentBrown2
        /* 2264, //*/ ImageIds::paletteMapTranslucentBrown0,             // translucentBrown0
        /* 2265, //*/ ImageIds::paletteMapTranslucentMutedRed1,          // translucentMutedRed1
        /* 2266, //*/ ImageIds::paletteMapTranslucentMutedRed2,          // translucentMutedRed2
        /* 2267, //*/ ImageIds::paletteMapTranslucentMutedRed0,          // translucentMutedRed0
        /* 2268, //*/ ImageIds::paletteMapTranslucentWhite1,             // translucentWhite1
        /* 2269, //*/ ImageIds::paletteMapTranslucentWhite2,             // translucentWhite2
        /* 2270, //*/ ImageIds::paletteMapTranslucentWhite0,             // translucentWhite0
        /* 2271, //*/ ImageIds::paletteMapTranslucentAmber1,             // translucentAmber1
        /* 2272, //*/ ImageIds::paletteMapTranslucentAmber2,             // translucentAmber2
        /* 2273, //*/ ImageIds::paletteMapTranslucentAmber0,             // translucentAmber0
        /* 2274, //*/ ImageIds::paletteMapUnk74,                         // unk74
        /* 2275, //*/ ImageIds::paletteMapUnk75,                         // unk75
        /* 2276, //*/ ImageIds::paletteMapUnk76,                         // unk76
        /* 2277, //*/ ImageIds::paletteMapUnk77,                         // unk77
        /* 2278, //*/ ImageIds::paletteMapUnk78,                         // unk78
        /* 2279, //*/ ImageIds::paletteMapUnk79,                         // unk79
        /* 2280, //*/ ImageIds::paletteMapUnk7A,                         // unk7A
        /* 2281, //*/ ImageIds::paletteMapUnk7B,                         // unk7B
        /* 2282, //*/ ImageIds::paletteMapUnk7C,                         // unk7C
        /* 2283, //*/ ImageIds::paletteMapUnk7D,                         // unk7D
        /* 2284, //*/ ImageIds::paletteMapUnk7E,                         // unk7E
        /* 2285, //*/ ImageIds::paletteMapUnk7F,                         // unk7F
        /* 2286, //*/ ImageIds::paletteMapUnk80,                         // unk80
        /* 2287, //*/ ImageIds::paletteMapUnk81,                         // unk81
        /* 2288, //*/ ImageIds::paletteMapUnk82,                         // unk82
        /* 2289, //*/ ImageIds::paletteMapUnk83,                         // unk83
        /* 2290, //*/ ImageIds::paletteMapUnk84,                         // unk84
        /* 2291, //*/ ImageIds::paletteMapUnk85,                         // unk85
        /* 2292, //*/ ImageIds::paletteMapUnk86,                         // unk86
        /* 2293, //*/ ImageIds::paletteMapUnk87,                         // unk87
        /* 2294, //*/ ImageIds::paletteMapUnk88,                         // unk88
        /* 2295, //*/ ImageIds::paletteMapUnk89,                         // unk89
        /* 2296, //*/ ImageIds::paletteMapUnk8A,                         // unk8A
        /* 2297, //*/ ImageIds::paletteMapUnk8B,                         // unk8B
        /* 2298, //*/ ImageIds::paletteMapUnk8C,                         // unk8C
        /* 2299, //*/ ImageIds::paletteMapUnk8D,                         // unk8D
        /* 2300, //*/ ImageIds::paletteMapUnk8E,                         // unk8E
        /* 2301, //*/ ImageIds::paletteMapUnk8F,                         // unk8F
        /* 2302, //*/ ImageIds::paletteMapUnk90,                         // unk90
        /* 2303, //*/ ImageIds::paletteMapUnk91,                         // unk91
        /* 2304, //*/ ImageIds::paletteMapUnk92                          // unk92
    };




    // 0x0050B8C8
    static std::array<uint32_t, enumValue(ExtColour::max)> _paletteToG1Offset = kDefaultPaletteToG1Offset;
    // static loco_global<uint32_t[enumValue(ExtColour::max)], 0x050B8C8> _paletteToG1Offset;


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
