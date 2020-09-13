#pragma once

#include "../Types.hpp"
#include <cstdint>
#include <limits>

namespace openloco
{
    struct Thing;
    struct vehicle_base;
    struct misc_thing;

    namespace thing_id
    {
        constexpr thing_id_t null = std::numeric_limits<thing_id_t>::max();
    }

    enum class thing_base_type : uint8_t
    {
        vehicle = 0,
        misc
    };

#pragma pack(push, 1)
    struct thing_base
    {
        thing_base_type base_type;

        void moveTo(loc16 loc);
        void invalidateSprite();
    };

    // Max size of a thing. Use when needing to know thing size
    struct Thing : thing_base
    {
    public:
        uint8_t type;
        uint8_t pad_02;
        uint8_t pad_03;
        thing_id_t next_thing_id; // 0x04
        uint8_t pad_06[0x09 - 0x06];
        uint8_t var_09;
        uint8_t pad_0A[0x0C - 0x0A];
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
        uint8_t pad_20[0x80 - 0x20];
        template<typename TType, thing_base_type TClass>
        TType* as() const
        {
            return base_type == TClass ? (TType*)this : nullptr;
        }

    public:
        vehicle_base* asVehicle() const { return as<vehicle_base, thing_base_type::vehicle>(); }
        misc_thing* asMisc() const { return as<misc_thing, thing_base_type::misc>(); }
    };
    static_assert(sizeof(Thing) == 0x80);
#pragma pack(pop)
}
