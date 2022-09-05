#include "Vector.hpp"
#include "../Interop/Interop.hpp"

namespace OpenLoco::Math::Vector
{
    static OpenLoco::Interop::loco_global<uint16_t[2048], 0x00500B50> _vehicle_arr_500B50;

    // 0x004BE368
    uint16_t fastSquareRoot(uint32_t distance)
    {
        uint8_t i = 10;
        for (; distance > 4096; --i, distance >>= 2)
            ;

        return _vehicle_arr_500B50[(distance & 0xFFE) >> 1] >> i;
    }
}
