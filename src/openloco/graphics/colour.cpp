#include "../interop/interop.hpp"
#include "colours.h"
#include <cassert>

using namespace openloco::interop;

namespace openloco::colour
{

    loco_global<uint8_t[32][8], 0x01136BA0> _colour_map_a;
    loco_global<uint8_t[32][8], 0x01136C98> _colour_map_b;

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
