#include "Colour.h"
#include "../Interop/Interop.hpp"
#include "Gfx.h"
#include <array>
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Colours
{

    loco_global<uint8_t[32][8], 0x01136BA0> _colour_map_a;
    loco_global<uint8_t[32][8], 0x01136C98> _colour_map_b;

    // 0x005045FA
    static constexpr std::array<ExtColour, 31> _translucentColourMap = {
        ExtColour::translucentGrey1,
        ExtColour::translucentGrey1,
        ExtColour::translucentWhite1,
        ExtColour::translucentMutedPurple1,
        ExtColour::translucentMutedPurple1,
        ExtColour::translucentPurple1,
        ExtColour::translucentBlue1,
        ExtColour::translucentBlue1,
        ExtColour::translucentMutedTeal1,
        ExtColour::translucentMutedTeal1,
        ExtColour::translucentGreen1,
        ExtColour::translucentMutedSeaGreen1,
        ExtColour::translucentMutedGrassGreen1,
        ExtColour::translucentGreen1,
        ExtColour::translucentMutedAvocadoGreen1,
        ExtColour::translucentMutedOliveGreen1,
        ExtColour::translucentYellow1,
        ExtColour::translucentYellow1,
        ExtColour::translucentOrange1,
        ExtColour::translucentAmber1,
        ExtColour::translucentOrange1,
        ExtColour::translucentMutedYellow1,
        ExtColour::translucentMutedYellow1,
        ExtColour::translucentBrown1,
        ExtColour::translucentMutedOrange1,
        ExtColour::translucentMutedDarkRed1,
        ExtColour::translucentRed1,
        ExtColour::translucentRed1,
        ExtColour::translucentPink1,
        ExtColour::translucentPink1,
        ExtColour::translucentMutedRed1,
    };

    void initColourMap()
    {
        // TODO: create a list of tuples with colour and image id

        for (uint32_t i = 0; i < 31; i++)
        {
            const auto c = static_cast<Colour>(i);
            auto paletteMap = Gfx::getPaletteMapForColour(toExt(c));
            if (!paletteMap)
            {
                continue;
            }
            auto& map = *paletteMap;
            _colour_map_a[i][0] = map[PaletteIndex::primaryRemap2];

            _colour_map_a[i][1] = map[PaletteIndex::primaryRemap3];
            _colour_map_a[i][2] = map[PaletteIndex::primaryRemap4];
            _colour_map_a[i][3] = map[PaletteIndex::primaryRemap5];

            _colour_map_a[i][4] = map[PaletteIndex::primaryRemap6];
            _colour_map_a[i][5] = map[PaletteIndex::primaryRemap7];
            _colour_map_a[i][6] = map[PaletteIndex::primaryRemap8];
            _colour_map_a[i][7] = map[PaletteIndex::primaryRemap9];

            _colour_map_b[i][8 - 8] = map[PaletteIndex::primaryRemapA];
            _colour_map_b[i][9 - 8] = map[PaletteIndex::primaryRemapB];
        }
    }

    uint8_t getShade(Colour colour, uint8_t shade)
    {
        assert(enumValue(colour) <= 31);

        if (shade < 8)
        {
            return _colour_map_a[enumValue(colour)][shade];
        }

        return _colour_map_b[enumValue(colour)][shade - 8];
    }

    // 0x005045FA
    ExtColour getTranslucent(Colour colour)
    {
        return _translucentColourMap[enumValue(colour)];
    }
}
