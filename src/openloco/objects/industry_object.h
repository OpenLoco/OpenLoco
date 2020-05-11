#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct industry_object
    {
        string_id name;
        uint8_t pad_02[0xCA - 0x02];
        uint16_t designed; // 0xCA start year
        uint16_t obsolete; // 0xCC end year
        uint8_t var_CE;
        uint8_t cost_ind;  // 0xCF
        int16_t cost_fact; // 0xD0
        uint8_t pad_D2[0xDE - 0xD2];
        uint8_t produced_cargo_type[2]; // 0xDE (0xFF = null)
        uint8_t received_cargo_type[3]; // 0xE0 (0xFF = null)
        uint32_t var_E4;
        uint8_t pad_E8[0xEA - 0xE8];
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t pad_EC;
        uint8_t var_ED;
        uint8_t var_EE;
        uint8_t var_EF;
        uint8_t var_F0;
    };
#pragma pack(pop)
}
