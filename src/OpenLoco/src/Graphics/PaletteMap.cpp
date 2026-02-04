#include "PaletteMap.h"
#include "Gfx.h"
#include "ImageIds.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

namespace OpenLoco::Gfx::PaletteMap
{
    static constexpr std::array<uint32_t, enumValue(ExtColour::max)> kDefaultPaletteToG1Offset = {
        ImageIds::paletteMapBlack,                         // black
        ImageIds::paletteMapGrey,                          // grey
        ImageIds::paletteMapWhite,                         // white
        ImageIds::paletteMapMutedDarkPurple,               // mutedDarkPurple
        ImageIds::paletteMapMutedPurple,                   // mutedPurple
        ImageIds::paletteMapPurple,                        // purple
        ImageIds::paletteMapDarkBlue,                      // darkBlue
        ImageIds::paletteMapBlue,                          // blue
        ImageIds::paletteMapMutedDarkTeal,                 // mutedDarkTeal
        ImageIds::paletteMapMutedTeal,                     // mutedTeal
        ImageIds::paletteMapDarkGreen,                     // darkGreen
        ImageIds::paletteMapMutedSeaGreen,                 // mutedSeaGreen
        ImageIds::paletteMapMutedGrassGreen,               // mutedGrassGreen
        ImageIds::paletteMapGreen,                         // green
        ImageIds::paletteMapMutedAvocadoGreen,             // mutedAvocadoGreen
        ImageIds::paletteMapMutedOliveGreen,               // mutedOliveGreen
        ImageIds::paletteMapYellow,                        // yellow
        ImageIds::paletteMapDarkYellow,                    // darkYellow
        ImageIds::paletteMapOrange,                        // orange
        ImageIds::paletteMapAmber,                         // amber
        ImageIds::paletteMapDarkOrange,                    // darkOrange
        ImageIds::paletteMapMutedDarkYellow,               // mutedDarkYellow
        ImageIds::paletteMapMutedYellow,                   // mutedYellow
        ImageIds::paletteMapBrown,                         // brown
        ImageIds::paletteMapMutedOrange,                   // mutedOrange
        ImageIds::paletteMapMutedDarkRed,                  // mutedDarkRed
        ImageIds::paletteMapDarkRed,                       // darkRed
        ImageIds::paletteMapRed,                           // red
        ImageIds::paletteMapDarkPink,                      // darkPink
        ImageIds::paletteMapPink,                          // pink
        ImageIds::paletteMapMutedRed,                      // mutedRed
        ImageIds::paletteMapClear,                         // clear
        0,                                                 // water (no palette map will be loaded from water object)
        ImageIds::paletteMapUnk21,                         // unk21
        ImageIds::paletteMapUnk22,                         // unk22
        ImageIds::paletteMapUnk23,                         // unk23
        ImageIds::paletteMapUnk24,                         // unk24
        ImageIds::paletteMapUnk25,                         // unk25
        ImageIds::paletteMapUnk26,                         // unk26
        ImageIds::paletteMapUnk27,                         // unk27
        ImageIds::paletteMapUnk28,                         // unk28
        ImageIds::paletteMapUnk29,                         // unk29
        ImageIds::paletteMapUnk2A,                         // unk2A
        ImageIds::paletteMapUnk2B,                         // unk2B
        ImageIds::paletteMapUnk2C,                         // unk2C
        ImageIds::paletteMapUnk2D,                         // unk2D
        ImageIds::paletteMapUnk2E,                         // unk2E
        ImageIds::paletteMapUnk2F,                         // unk2F
        ImageIds::paletteMapUnk30,                         // unk30
        ImageIds::paletteMapUnk31,                         // unk31
        ImageIds::paletteMapUnk32,                         // unk32
        ImageIds::paletteMapUnk33,                         // unk33
        ImageIds::paletteMapUnk34,                         // unk34
        ImageIds::paletteMapTranslucentGrey1,              // translucentGrey1
        ImageIds::paletteMapTranslucentGrey2,              // translucentGrey2
        ImageIds::paletteMapTranslucentGrey0,              // translucentGrey0
        ImageIds::paletteMapTranslucentBlue1,              // translucentBlue1
        ImageIds::paletteMapTranslucentBlue2,              // translucentBlue2
        ImageIds::paletteMapTranslucentBlue0,              // translucentBlue0
        ImageIds::paletteMapTranslucentMutedDarkRed1,      // translucentMutedDarkRed1
        ImageIds::paletteMapTranslucentMutedDarkRed2,      // translucentMutedDarkRed2
        ImageIds::paletteMapTranslucentMutedDarkRed0,      // translucentMutedDarkRed0
        ImageIds::paletteMapTranslucentMutedSeaGreen1,     // translucentMutedSeaGreen1
        ImageIds::paletteMapTranslucentMutedSeaGreen2,     // translucentMutedSeaGreen2
        ImageIds::paletteMapTranslucentMutedSeaGreen0,     // translucentMutedSeaGreen0
        ImageIds::paletteMapTranslucentMutedPurple1,       // translucentMutedPurple1
        ImageIds::paletteMapTranslucentMutedPurple2,       // translucentMutedPurple2
        ImageIds::paletteMapTranslucentMutedPurple0,       // translucentMutedPurple0
        ImageIds::paletteMapTranslucentMutedOliveGreen1,   // translucentMutedOliveGreen1
        ImageIds::paletteMapTranslucentMutedOliveGreen2,   // translucentMutedOliveGreen2
        ImageIds::paletteMapTranslucentMutedOliveGreen0,   // translucentMutedOliveGreen0
        ImageIds::paletteMapTranslucentMutedYellow1,       // translucentMutedYellow1
        ImageIds::paletteMapTranslucentMutedYellow2,       // translucentMutedYellow2
        ImageIds::paletteMapTranslucentMutedYellow0,       // translucentMutedYellow0
        ImageIds::paletteMapTranslucentYellow1,            // translucentYellow1
        ImageIds::paletteMapTranslucentYellow2,            // translucentYellow2
        ImageIds::paletteMapTranslucentYellow0,            // translucentYellow0
        ImageIds::paletteMapTranslucentMutedGrassGreen1,   // translucentMutedGrassGreen1
        ImageIds::paletteMapTranslucentMutedGrassGreen2,   // translucentMutedGrassGreen2
        ImageIds::paletteMapTranslucentMutedGrassGreen0,   // translucentMutedGrassGreen0
        ImageIds::paletteMapTranslucentMutedAvocadoGreen1, // translucentMutedAvocadoGreen1
        ImageIds::paletteMapTranslucentMutedAvocadoGreen2, // translucentMutedAvocadoGreen2
        ImageIds::paletteMapTranslucentMutedAvocadoGreen0, // translucentMutedAvocadoGreen0
        ImageIds::paletteMapTranslucentGreen1,             // translucentGreen1
        ImageIds::paletteMapTranslucentGreen2,             // translucentGreen2
        ImageIds::paletteMapTranslucentGreen0,             // translucentGreen0
        ImageIds::paletteMapTranslucentMutedOrange1,       // translucentMutedOrange1
        ImageIds::paletteMapTranslucentMutedOrange2,       // translucentMutedOrange2
        ImageIds::paletteMapTranslucentMutedOrange0,       // translucentMutedOrange0
        ImageIds::paletteMapTranslucentPurple1,            // translucentPurple1
        ImageIds::paletteMapTranslucentPurple2,            // translucentPurple2
        ImageIds::paletteMapTranslucentPurple0,            // translucentPurple0
        ImageIds::paletteMapTranslucentRed1,               // translucentRed1
        ImageIds::paletteMapTranslucentRed2,               // translucentRed2
        ImageIds::paletteMapTranslucentRed0,               // translucentRed0
        ImageIds::paletteMapTranslucentOrange1,            // translucentOrange1
        ImageIds::paletteMapTranslucentOrange2,            // translucentOrange2
        ImageIds::paletteMapTranslucentOrange0,            // translucentOrange0
        ImageIds::paletteMapTranslucentMutedTeal1,         // translucentMutedTeal1
        ImageIds::paletteMapTranslucentMutedTeal2,         // translucentMutedTeal2
        ImageIds::paletteMapTranslucentMutedTeal0,         // translucentMutedTeal0
        ImageIds::paletteMapTranslucentPink1,              // translucentPink1
        ImageIds::paletteMapTranslucentPink2,              // translucentPink2
        ImageIds::paletteMapTranslucentPink0,              // translucentPink0
        ImageIds::paletteMapTranslucentBrown1,             // translucentBrown1
        ImageIds::paletteMapTranslucentBrown2,             // translucentBrown2
        ImageIds::paletteMapTranslucentBrown0,             // translucentBrown0
        ImageIds::paletteMapTranslucentMutedRed1,          // translucentMutedRed1
        ImageIds::paletteMapTranslucentMutedRed2,          // translucentMutedRed2
        ImageIds::paletteMapTranslucentMutedRed0,          // translucentMutedRed0
        ImageIds::paletteMapTranslucentWhite1,             // translucentWhite1
        ImageIds::paletteMapTranslucentWhite2,             // translucentWhite2
        ImageIds::paletteMapTranslucentWhite0,             // translucentWhite0
        ImageIds::paletteMapTranslucentAmber1,             // translucentAmber1
        ImageIds::paletteMapTranslucentAmber2,             // translucentAmber2
        ImageIds::paletteMapTranslucentAmber0,             // translucentAmber0
        ImageIds::paletteMapUnk74,                         // unk74
        ImageIds::paletteMapUnk75,                         // unk75
        ImageIds::paletteMapUnk76,                         // unk76
        ImageIds::paletteMapUnk77,                         // unk77
        ImageIds::paletteMapUnk78,                         // unk78
        ImageIds::paletteMapUnk79,                         // unk79
        ImageIds::paletteMapUnk7A,                         // unk7A
        ImageIds::paletteMapUnk7B,                         // unk7B
        ImageIds::paletteMapUnk7C,                         // unk7C
        ImageIds::paletteMapUnk7D,                         // unk7D
        ImageIds::paletteMapUnk7E,                         // unk7E
        ImageIds::paletteMapUnk7F,                         // unk7F
        ImageIds::paletteMapUnk80,                         // unk80
        ImageIds::paletteMapUnk81,                         // unk81
        ImageIds::paletteMapUnk82,                         // unk82
        ImageIds::paletteMapUnk83,                         // unk83
        ImageIds::paletteMapUnk84,                         // unk84
        ImageIds::paletteMapUnk85,                         // unk85
        ImageIds::paletteMapUnk86,                         // unk86
        ImageIds::paletteMapUnk87,                         // unk87
        ImageIds::paletteMapUnk88,                         // unk88
        ImageIds::paletteMapUnk89,                         // unk89
        ImageIds::paletteMapUnk8A,                         // unk8A
        ImageIds::paletteMapUnk8B,                         // unk8B
        ImageIds::paletteMapUnk8C,                         // unk8C
        ImageIds::paletteMapUnk8D,                         // unk8D
        ImageIds::paletteMapUnk8E,                         // unk8E
        ImageIds::paletteMapUnk8F,                         // unk8F
        ImageIds::paletteMapUnk90,                         // unk90
        ImageIds::paletteMapUnk91,                         // unk91
        ImageIds::paletteMapUnk92                          // unk92
    };
    // 0x0050B8C8
    static std::array<uint32_t, enumValue(ExtColour::max)> _paletteToG1Offset = kDefaultPaletteToG1Offset;

    // Default immutable palette map.
    static const auto _defaultPaletteMapBuffer = [] {
        Buffer<kDefaultSize> data;
        std::iota(data.begin(), data.end(), 0);
        return data;
    }();

    // This buffer is used when sprites are drawn with a secondary palette.
    // TODO: Move this into the drawing context.
    static thread_local auto _secondaryPaletteMapBuffer = _defaultPaletteMapBuffer;

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
