#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct ClimateObject
    {
        string_id name; // 0x00
        uint8_t var_02; // 0x02
        uint8_t var_03; // 0x03
        uint8_t var_04; // 0x04
        uint8_t var_05; // 0x05
        uint8_t var_06; // 0x06
        uint8_t var_07; // 0x07
        uint8_t var_08; // 0x08
    };
#pragma pack(pop)
}
