#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct SteamObject
    {
        string_id name; // 0x00 probably not confirmed
        uint8_t pad_02[0x5 - 0x2];
        uint8_t var_05;
        uint8_t var_06;
        uint8_t var_07;
        uint16_t var_08;
        uint32_t var_0A;
        uint32_t baseImageId; // 0x0E
        uint8_t pad_12[0x16 - 0x12];
        uint8_t* var_16;
        uint8_t* var_1A;
        uint8_t sound_effect; // 0x1E probably not confirmed
        uint8_t var_1F[9];    // size tbc
    };
#pragma pack(pop)
}
