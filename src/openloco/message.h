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
        uint16_t item_id;   // 0xCA
        uint16_t var_CC;
        uint8_t pad_CE[0xD0 - 0xCE];
        uint32_t date; // 0xD0
    };
#pragma pack(pop)
}
