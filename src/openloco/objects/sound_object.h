#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct sound_object
    {
        string_id name;
        void* data;
        uint8_t pad_06[0x2];
        uint32_t volume; // 0x08
    };
#pragma pack(pop)
}
