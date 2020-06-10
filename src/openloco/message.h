#pragma once
#include "localisation/stringmgr.h"
#include "map/tile.h"

namespace openloco
{
    using message_id_t = uint16_t;
#pragma pack(push, 1)
    struct message
    {
        uint8_t var_00;
        char messageString[198];
        uint8_t companyId; // 0xC7
        uint16_t var_C8;
        uint8_t pad_CA[0xD0 - 0xCA];
        uint16_t date; // 0xD0
        uint8_t pad_D2[0xD4 - 0xD2];
    };
#pragma pack(pop)
}
