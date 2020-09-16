#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace sound_object_id
    {
        constexpr sound_object_id_t null = 0xFF;
    }
#pragma pack(push, 1)
    struct sound_object
    {
        string_id name;
        void* data;
        uint8_t var_06;
        uint8_t pad_07;
        uint32_t volume; // 0x08
    };
#pragma pack(pop)
}
