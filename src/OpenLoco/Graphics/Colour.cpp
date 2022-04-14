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
    static constexpr std::array<uint8_t, 32> _translucentColourMap = {
        PaletteIndex::index_35,
        PaletteIndex::index_35,
        PaletteIndex::index_6E,
        PaletteIndex::index_41,
        PaletteIndex::index_41,
        PaletteIndex::index_59,
        PaletteIndex::index_38,
        PaletteIndex::index_38,
        PaletteIndex::index_62,
        PaletteIndex::index_62,
        PaletteIndex::index_53,
        PaletteIndex::index_3E,
        PaletteIndex::index_4D,
        PaletteIndex::index_53,
        PaletteIndex::index_50,
        PaletteIndex::index_44,
        PaletteIndex::index_4A,
        PaletteIndex::index_4A,
        PaletteIndex::index_5F,
        PaletteIndex::index_71,
        PaletteIndex::index_5F,
        PaletteIndex::index_47,
        PaletteIndex::index_47,
        PaletteIndex::index_68,
        PaletteIndex::index_56,
        PaletteIndex::index_3B,
        PaletteIndex::index_5C,
        PaletteIndex::index_5C,
        PaletteIndex::index_65,
        PaletteIndex::index_65,
        PaletteIndex::index_6B,
    };

    void initColourMap()
    {
        // TODO: create a list of tuples with colour and image id

        for (uint32_t i = 0; i < 31; i++)
        {
            assert(i + 2170 < 2201);
            auto image = Gfx::getG1Element(2170 + i);
            _colour_map_a[i][0] = image->offset[9];

            _colour_map_a[i][1] = image->offset[246];
            _colour_map_a[i][2] = image->offset[247];
            _colour_map_a[i][3] = image->offset[248];

            _colour_map_a[i][4] = image->offset[249];
            _colour_map_a[i][5] = image->offset[250];
            _colour_map_a[i][6] = image->offset[251];
            _colour_map_a[i][7] = image->offset[252];

            _colour_map_b[i][8 - 8] = image->offset[253];
            _colour_map_b[i][9 - 8] = image->offset[254];
            _colour_map_b[i][10 - 8] = image->offset[255];
            _colour_map_b[i][11 - 8] = image->offset[256];
        }
    }

    uint8_t getShade(Colour2 colour, uint8_t shade)
    {
        assert(enumValue(colour) <= 31);

        if (shade < 8)
        {
            return _colour_map_a[enumValue(colour)][shade];
        }

        return _colour_map_b[enumValue(colour)][shade - 8];
    }

    // 0x005045FA
    PaletteIndex_t getTranslucent(Colour2 colour)
    {
        return _translucentColourMap[enumValue(colour)];
    }
}
