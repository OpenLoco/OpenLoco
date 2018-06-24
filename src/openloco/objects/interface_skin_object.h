#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct interface_skin_object
    {
        string_id name;
        uint32_t img;
        uint8_t colour_06;
        uint8_t colour_07;
        uint8_t colour_08;
        uint8_t colour_09;
        uint8_t colour_0A;
        uint8_t colour_0B;
        uint8_t colour_0C;
        uint8_t colour_0D;
        uint8_t colour_0E;
        uint8_t colour_0F;
        uint8_t colour_10;
        uint8_t colour_11;
        uint8_t colour_12;
        uint8_t colour_13;
        uint8_t colour_14;
        uint8_t colour_15;
        uint8_t colour_16;
        uint8_t colour_17;
    };
#pragma pack(pop)
}
