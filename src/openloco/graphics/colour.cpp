#include "../interop/interop.hpp"
#include "colours.h"
#include "gfx.h"
#include <cassert>

using namespace openloco::interop;

namespace openloco::colour
{

    loco_global<uint8_t[32][8], 0x01136BA0> _colour_map_a;
    loco_global<uint8_t[32][8], 0x01136C98> _colour_map_b;

    void init_colour_map()
    {
        // TODO: create a list of tuples with colour and image id

        for (uint32_t i = 0; i < 31; i++)
        {
            assert(i + 2170 < 2201);
            auto image = gfx::get_g1_element(2170 + i);
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

    uint8_t get_shade(colour_t colour, uint8_t shade)
    {
        assert(colour <= 31);

        if (shade < 8)
        {
            return _colour_map_a[colour][shade];
        }

        return _colour_map_b[colour][shade - 8];
    }
}
