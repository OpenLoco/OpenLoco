#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct SteamObject;

    struct Exhaust : EffectEntity
    {
        uint16_t frameNum;          // 0x26
        int16_t stationaryProgress; // 0x28
        uint16_t windProgress;      // 0x32
        int16_t var_34;
        int16_t var_36;
        uint8_t objectId; // 0x49

        const SteamObject* getObject() const;
        void update();

        static Exhaust* create(World::Pos3 loc, uint8_t type);
        bool isSubObjType1() const { return objectId & (1 << 7); } // Used for steam / steampuff
    };
    static_assert(sizeof(Exhaust) <= sizeof(Entity));
}
