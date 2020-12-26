#pragma once

#include "../Objects/SteamObject.h"
#include "Thing.h"

namespace OpenLoco
{
    struct smoke;
    struct exhaust;

    enum class MiscThingType : uint8_t
    {
        exhaust = 0,
        smoke = 8
    };

#pragma pack(push, 1)
    struct MiscBase : thing_base
    {
    private:
        template<typename TType, MiscThingType TClass>
        TType* as() const
        {
            return getSubType() == TClass ? (TType*)this : nullptr;
        }

    public:
        MiscThingType getSubType() const { return MiscThingType(thing_base::getSubType()); }
        void setSubType(const MiscThingType newType) { thing_base::setSubType(static_cast<uint8_t>(newType)); }
        smoke* as_smoke() const { return as<smoke, MiscThingType::smoke>(); }
        exhaust* as_exhaust() const { return as<exhaust, MiscThingType::exhaust>(); }
    };

    struct exhaust : MiscBase
    {
        uint8_t pad_20[0x26 - 0x20];
        int16_t var_26;
        int16_t var_28;
        uint8_t pad_2A[0x32 - 0x2A];
        int16_t var_32;
        int16_t var_34;
        int16_t var_36;
        uint8_t pad_38[0x49 - 0x38];
        uint8_t object_id; // 0x49

        steam_object* object() const;

        static exhaust* create(loc16 loc, uint8_t type);
    };

    struct smoke : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t var_28;

        static smoke* create(loc16 loc);
    };
#pragma pack(pop)
}
