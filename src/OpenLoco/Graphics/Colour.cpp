#include "Colour.h"
#include "../Interop/Interop.hpp"
#include "Gfx.h"
#include "PaletteMap.h"
#include <array>
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Colours
{

    loco_global<uint8_t[32][8], 0x01136BA0> _colourMapA;
    loco_global<uint8_t[32][8], 0x01136C98> _colourMapB;

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

    static constexpr std::array<ExtColour, 31> _shadowColourMap = {
        ExtColour::null,
        ExtColour::unk21,
        ExtColour::unk22,
        ExtColour::unk23,
        ExtColour::unk24,
        ExtColour::unk25,
        ExtColour::unk26,
        ExtColour::unk27,
        ExtColour::unk28,
        ExtColour::unk29,
        ExtColour::unk2A,
        ExtColour::unk2B,
        ExtColour::unk2C,
        ExtColour::unk2D,
        ExtColour::unk2E,
        ExtColour::unk2F,
        ExtColour::unk30,
        ExtColour::unk31,
        ExtColour::unk32,
        ExtColour::unk33,
        ExtColour::unk34,
        ExtColour::translucentGrey1,
        ExtColour::translucentGrey2,
        ExtColour::translucentGrey0,
        ExtColour::translucentBlue1,
        ExtColour::translucentBlue2,
        ExtColour::translucentBlue0,
        ExtColour::translucentMutedDarkRed1,
        ExtColour::translucentMutedDarkRed2,
        ExtColour::translucentMutedDarkRed0,
        ExtColour::translucentMutedSeaGreen1,
    };

    void initColourMap()
    {
        // TODO: create a list of tuples with colour and image id

        for (uint32_t i = 0; i < 31; i++)
        {
            const auto c = static_cast<Colour>(i);
            auto paletteMap = Gfx::PaletteMap::getForColour(toExt(c));
            if (!paletteMap)
            {
                continue;
            }
            auto& map = *paletteMap;
            _colourMapA[i][0] = map[PaletteIndex::primaryRemap2];

            _colourMapA[i][1] = map[PaletteIndex::primaryRemap3];
            _colourMapA[i][2] = map[PaletteIndex::primaryRemap4];
            _colourMapA[i][3] = map[PaletteIndex::primaryRemap5];

            _colourMapA[i][4] = map[PaletteIndex::primaryRemap6];
            _colourMapA[i][5] = map[PaletteIndex::primaryRemap7];
            _colourMapA[i][6] = map[PaletteIndex::primaryRemap8];
            _colourMapA[i][7] = map[PaletteIndex::primaryRemap9];

            _colourMapB[i][8 - 8] = map[PaletteIndex::primaryRemapA];
            _colourMapB[i][9 - 8] = map[PaletteIndex::primaryRemapB];
        }
    }

    uint8_t getShade(Colour colour, uint8_t shade)
    {
        assert(enumValue(colour) <= 31);

        if (shade < 8)
        {
            return _colourMapA[enumValue(colour)][shade];
        }

        return _colourMapB[enumValue(colour)][shade - 8];
    }

    // 0x005045FA
    ExtColour getTranslucent(Colour colour)
    {
        return _translucentColourMap[enumValue(colour)];
    }

    ExtColour getShadow(Colour colour)
    {
        return _shadowColourMap[enumValue(colour)];
    }
}
