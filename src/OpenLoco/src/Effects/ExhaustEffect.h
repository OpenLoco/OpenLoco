#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct SteamObject;

#pragma pack(push, 1)

    struct Exhaust : EffectEntity
    {
        uint8_t pad_24[0x26 - 0x24];
        uint16_t frameNum;          // 0x26
        int16_t stationaryProgress; // 0x28
        uint8_t pad_2A[0x32 - 0x2A];
        uint16_t windProgress; // 0x32
        int16_t var_34;
        int16_t var_36;
        uint8_t pad_38[0x49 - 0x38];
        uint8_t objectId; // 0x49

        const SteamObject* getObject() const;
        void update();

        static Exhaust* create(World::Pos3 loc, uint8_t type);
        bool isSubObjType1() const { return objectId & (1 << 7); } // Used for steam / steampuff
    };
    static_assert(sizeof(Exhaust) == 0x4A);

#pragma pack(pop)
}
