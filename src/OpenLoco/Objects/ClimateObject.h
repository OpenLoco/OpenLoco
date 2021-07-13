#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct ClimateObject
    {
        string_id name;          // 0x00
        uint8_t firstSeason;     // 0x02
        uint8_t seasonLength[4]; // 0x03
        uint8_t winterSnowLine;  // 0x07
        uint8_t summerSnowLine;  // 0x08
    };
#pragma pack(pop)
}
