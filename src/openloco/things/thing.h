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

    enum class thing_type : uint8_t
    {
        exhaust = 0,
        vehicle_1,
        vehicle_2,
        vehicle_bogie,
        vehicle_body_end,
        vehicle_body_cont,
        vehicle_6,
        smoke = 8
    };

#pragma pack(push, 1)
    struct thing_base
    {
        uint8_t var_00;
        thing_type type;
        uint8_t pad_02;
        uint8_t pad_03;
        thing_id_t next_thing_id; // 0x04
        uint8_t pad_06[0x09 - 0x06];
        uint8_t var_09;
        thing_id_t id; // 0x0A
        uint8_t pad_0C[0x0E - 0x0C];
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

        void move_to(loc16 loc);
        void invalidate_sprite();
    };

    struct vehicle_bogie;
    struct vehicle_body;
    struct smoke;
    struct exhaust;
    // Max size of a thing. Use when needing to know thing size
    struct thing : thing_base
    {
    private:
        uint8_t pad_20[128 - 0x20];
        template<typename TType, thing_type TClass>
        TType* as() const
        {
            return type == TClass ? (TType*)this : nullptr;
        }

    public:
        vehicle_bogie* as_vehicle_bogie() const { return as<vehicle_bogie, thing_type::vehicle_bogie>(); }
        vehicle_body* as_vehicle_body() const
        {
            auto vehicle = as<vehicle_body, thing_type::vehicle_body_end>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_body, thing_type::vehicle_body_cont>();
        }
        smoke* as_smoke() const { return as<smoke, thing_type::smoke>(); }
        exhaust* as_exahust() const { return as<exhaust, thing_type::exhaust>(); }
    };
#pragma pack(pop)
}
