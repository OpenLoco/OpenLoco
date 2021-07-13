#include "Unicode.h"
#include <cstdint>

namespace OpenLoco::Localisation
{
    utf32_t readCodePoint(utf8_t** string)
    {
        utf32_t read = 0;

        utf8_t* ptr = *string;

        if ((ptr[0] & 0b10000000) == 0)
        {
            read = ptr[0];
            *string += 1;
        }
        else if ((ptr[0] & 0b11100000) == 0b11000000)
        {
            read = ((ptr[0] & 0b11111) << 6) | (ptr[1] & 0b111111);
            *string += 2;
        }
        else if ((ptr[0] & 0b11110000) == 0b11100000)
        {
            read = ((ptr[0] & 0b1111) << 12) | ((ptr[1] & 0b111111) << 6) | (ptr[2] & 0b111111);
            *string += 3;
        }
        else if ((ptr[0] & 0b11111000) == 0b11110000)
        {
            read = ((ptr[0] & 0b111) << 18) | ((ptr[1] & 0b111111) << 12) | ((ptr[2] & 0b111111) << 6)
                | (ptr[3] & 0b111111);
            *string += 4;
        }

        return read;
    }
}
