#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct steam_object
    {
        string_id name; // 0x00 probably not confirmed
        uint8_t pad_02[0x5 - 0x2];
        uint8_t var_05;
        uint8_t var_06;
        uint8_t var_07;
        uint16_t var_08;
        uint8_t pad_0A[0x1E - 0xA];
        uint8_t sound_effect; // 0x1E probably not confirmed
        uint8_t var_1F[9];    // size tbc
    };
#pragma pack(pop)
}
