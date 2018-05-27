#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct airport_var_AE_object
    {
        int16_t x;      // 0x00
        int16_t y;      // 0x02
        int16_t z;      // 0x04
        uint16_t flags; // 0x06
    };

    struct airport_var_B2_object
    {
        uint8_t var_00;
        uint8_t var_01;
        uint8_t var_02;
        uint8_t var_03;
        uint32_t var_04;
        uint32_t var_08;
    };

    struct airport_object
    {
        string_id name;
        uint8_t pad_02[0x10 - 0x02];
        uint16_t var_10;
        uint8_t pad_12[0xAD - 0x12];
        uint8_t var_AD;
        airport_var_AE_object* var_AE;
        airport_var_B2_object* var_B2;
        uint8_t pad_B6[0xBA - 0xB6];
    };
#pragma pack(pop)
}
