#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    struct steam_object
    {
        string_id name;         // 0x00 probably not confirmed
        uint8_t pad_02[0x5 - 0x2];
        uint8_t var_05;
        uint8_t var_06;
        uint8_t var_07;
        uint8_t pad_08[0x1E - 0x8];
        uint8_t sound_effect;   // 0x1E probably not confirmed
        uint8_t pad_1F[0x28 - 0x1F];
    };
}
