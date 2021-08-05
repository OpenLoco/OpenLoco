#include "Trigonometry.hpp"
#include "../Interop/Interop.hpp"

namespace OpenLoco::Math::Trigonometry
{
    // Excel Function =SIN((A1/16384)*2*PI())*32768
    // Where A1 is 0 : 4095
    static Interop::loco_global<int16_t[directionPrecisionHigh / 4], 0x00501B50> _quarterSine;

    int32_t integerSinePrecisionHigh(uint16_t direction, int32_t magnitude)
    {
        // Build the full sine wave from the quarter wave by subtraction of direction/magnitude
        const auto sineIndex = ((direction & (1 << 12)) ? -direction : direction) & 0xFFF;
        const auto value = (direction & (1 << 13)) ? -_quarterSine[sineIndex] : _quarterSine[sineIndex];
        return value * magnitude / 0x8000;
    }

    int32_t integerCosinePrecisionHigh(uint16_t direction, int32_t magnitude)
    {
        // Cosine is Sine plus pi/2
        return integerSinePrecisionHigh(direction + directionPrecisionHigh / 4, magnitude);
    }
}
