#pragma once

#include "../objects/steam_object.h"
#include "thing.h"

namespace openloco
{
    struct smoke;
    struct exhaust;

    enum class misc_thing_type : uint8_t
    {
        exhaust = 0,
        smoke = 8
    };

#pragma pack(push, 1)
    struct misc_thing : thing_base
    {
        misc_thing_type type;
        uint8_t pad_02;
        uint8_t pad_03;
        thing_id_t next_thing_id; // 0x04
        uint8_t pad_06[0x09 - 0x06];
        uint8_t var_09;
        thing_id_t id; // 0x0A
        uint16_t var_0C;
        int16_t x; // 0x0E
        int16_t y; // 0x10
        int16_t z; // 0x12
        uint8_t var_14;
        uint8_t var_15;
        int16_t sprite_left;   // 0x16
        int16_t sprite_top;    // 0x18
        int16_t sprite_right;  // 0x1A
        int16_t sprite_bottom; // 0x1C
        uint8_t sprite_yaw;    // 0x1E
        uint8_t sprite_pitch;  // 0x1F

    private:
        template<typename TType, misc_thing_type TClass>
        TType* as() const
        {
            return type == TClass ? (TType*)this : nullptr;
        }

    public:
        smoke* as_smoke() const { return as<smoke, misc_thing_type::smoke>(); }
        exhaust* as_exhaust() const { return as<exhaust, misc_thing_type::exhaust>(); }
    };

    struct exhaust : misc_thing
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

    struct smoke : misc_thing
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t var_28;

        static smoke* create(loc16 loc);
    };
#pragma pack(pop)
}
