#include "Trigonometry.hpp"
#include "../Interop/Interop.hpp"

namespace OpenLoco::Math::Trigonometry
{
    // Excel Function =SIN((A1/16384)*2*PI())*32768
    // Where A1 is 0 : 4095
    static Interop::loco_global<int16_t[directionPrecisionHigh / 4], 0x00501B50> _quarterSine;

    int32_t integerSinePrecisionHigh(uint16_t direction, int32_t magnitude)
    {
        int16_t value = 0;
        // Build the full sine wave from the quarter wave by subtraction of direction/magnitude
        if (direction & (1 << 13))
        {
            if (direction & (1 << 12))
            {
                value = -_quarterSine[(-direction) & 0xFFF];
            }
            else
            {
                value = -_quarterSine[direction & 0xFFF];
            }
        }
        else
        {
            if (direction & (1 << 12))
            {
                value = _quarterSine[(-direction) & 0xFFF];
            }
            else
            {
                value = _quarterSine[direction & 0xFFF];
            }
        }
        return value * magnitude / 32768;
    }

    int32_t integerCosinePrecisionHigh(uint16_t direction, int32_t magnitude)
    {
        auto value = 0;
        // Build the full cosine wave from the quarter wave by subtraction of direction/magnitude
        if (direction & (1 << 13))
        {
            if (direction & (1 << 12))
            {
                value = _quarterSine[direction & 0xFFF];
            }
            else
            {
                value = -_quarterSine[(-direction) & 0xFFF];
            }
        }
        else
        {
            if (direction & (1 << 12))
            {
                value = -_quarterSine[direction & 0xFFF];
            }
            else
            {
                value = _quarterSine[(-direction) & 0xFFF];
            }
        }
        return value * magnitude / 32768;
    }
}
