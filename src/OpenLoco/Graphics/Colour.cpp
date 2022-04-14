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
        ExtColour::unk35,
        ExtColour::unk35,
        ExtColour::unk6E,
        ExtColour::unk41,
        ExtColour::unk41,
        ExtColour::unk59,
        ExtColour::unk38,
        ExtColour::unk38,
        ExtColour::unk62,
        ExtColour::unk62,
        ExtColour::unk53,
        ExtColour::unk3E,
        ExtColour::unk4D,
        ExtColour::unk53,
        ExtColour::unk50,
        ExtColour::unk44,
        ExtColour::unk4A,
        ExtColour::unk4A,
        ExtColour::unk5F,
        ExtColour::unk71,
        ExtColour::unk5F,
        ExtColour::unk47,
        ExtColour::unk47,
        ExtColour::unk68,
        ExtColour::unk56,
        ExtColour::unk3B,
        ExtColour::unk5C,
        ExtColour::unk5C,
        ExtColour::unk65,
        ExtColour::unk65,
        ExtColour::unk6B,
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
