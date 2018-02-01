#pragma once

#include "../objects/steam_object.h"
#include "thing.h"

namespace openloco
{
#pragma pack(push, 1)
    struct exahust : thing_base
    {
        uint8_t pad_20[0x26 - 0x20];
        int16_t var_26;
        int16_t var_28;
        uint8_t pad_2A[0x32 - 0x2A];
        int16_t var_32;
        int16_t var_34;
        int16_t var_36;
        uint8_t pad_38[0x49 - 0x38];
        uint8_t object_id;

        steam_object* object() const;

        static exahust* create(loc16 loc, uint8_t type);
    };

    struct smoke : thing_base
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t var_28;

        static smoke* create(loc16 loc);
    };
#pragma pack(pop)
}
