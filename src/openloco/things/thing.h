#pragma once

#include "../types.hpp"
#include <cstdint>
#include <limits>

namespace openloco
{
    using thing_id_t = uint16_t;

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
    struct thing;
    // Required due to no virtual functions being possible
    // and to prevent variables from being located in different places.
    struct thing_base
    {
        thing_base_type base_type;
        void move_to(loc16 loc);
        void invalidate_sprite();
    };

    struct vehicle_base;
    struct misc;

    // Max size of a thing. Use when needing to know thing size
    struct thing : thing_base
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
        uint8_t pad_01[128 - 0x1F];

    private:
        template<typename TType, thing_base_type TClass>
        TType* as() const
        {
            return base_type == TClass ? (TType*)this : nullptr;
        }

    public:
        void move_to(loc16 loc);
        void invalidate_sprite();
        vehicle_base * as_vehicle() const { return as<vehicle_base, thing_base_type::vehicle>(); }
        misc * as_misc() const { return as<misc, thing_base_type::vehicle>(); }
    };
#pragma pack(pop)
}
