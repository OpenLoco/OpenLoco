#pragma once

#include "../company.h"
#include "../objects/vehicle_object.h"
#include "thing.h"

namespace openloco
{
    namespace flags_5f
    {
        constexpr uint8_t breakdown_pending = 1 << 1;
        constexpr uint8_t broken_down = 1 << 2;
    }

#pragma pack(push, 1)
    struct vehicle : thing_base
    {
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint8_t pad_22[0x28 - 0x22];
        uint16_t var_28;
        uint8_t pad_2A[0x2C - 0x2A];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t pad_35[0x38 - 0x35];
        uint8_t var_38;
        uint8_t object_sprite_type; // 0x39
        thing_id_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x40 - 0x3C];
        uint16_t object_id; // 0x40
        uint8_t var_42;
        uint8_t pad_43;
        int16_t var_44;
        uint8_t var_46;
        uint8_t pad_47[0x4A - 0x47];
        uint16_t var_4A;
        uint8_t cargo_type; // 0x4C
        uint8_t pad_4D;
        uint16_t cargo_origin; // 0x4E
        uint8_t pad_50;
        uint8_t cargo_quantity; // 0x51
        uint8_t pad_52[0x54 - 0x52];
        uint8_t var_54;
        int8_t var_55;
        uint32_t var_56;
        uint8_t var_5A;
        uint8_t pad_5B[0x5D - 0x5B];
        uint8_t var_5D;
        uint8_t var_5E;
        uint8_t var_5F; // 0x5F (bit 1 = can break down)
        uint8_t pad_60[0x6A - 0x60];
        uint8_t var_6A;
        uint8_t pad_6B[0x73 - 0x6B];
        uint8_t var_73; // 0x73 (bit 0 = broken down)

        vehicle* next_vehicle();
        vehicle* next_car();
        vehicle_object* object() const;

        void update_head();
        void sub_4BA8D4();
        void secondary_animation_update();

    private:
        bool update();
        void sub_4BAA76();
        int32_t sub_4AA1D0();
        void animation_update();
        void sub_4AAB0B();
        void sub_4AC255(vehicle* back_bogie, vehicle* front_bogie);
        uint16_t sub_4BE368(uint32_t distance);
        uint8_t vehicle_body_update_sprite_pitch_steep_slopes(uint16_t xy_offset, int16_t z_offset);
        uint8_t vehicle_body_update_sprite_pitch(uint16_t xy_offset, int16_t z_offset);
        uint8_t vehicle_update_sprite_yaw_0(int16_t x_offset, int16_t y_offset);
        uint8_t vehicle_update_sprite_yaw_1(int16_t x_offset, int16_t y_offset);
        uint8_t vehicle_update_sprite_yaw_2(int16_t x_offset, int16_t y_offset);
        uint8_t vehicle_update_sprite_yaw_3(int16_t x_offset, int16_t y_offset);
        uint8_t vehicle_update_sprite_yaw_4(int16_t x_offset, int16_t y_offset);
        void steam_puffs_animation_update(uint8_t num, int8_t var_05);
        void diesel_exhaust1_animation_update(uint8_t num, int8_t var_05);
        void diesel_exhaust2_animation_update(uint8_t num, int8_t var_05);
        void electric_spark1_animation_update(uint8_t num, int8_t var_05);
        void electric_spark2_animation_update(uint8_t num, int8_t var_05);
        void ship_wake_animation_update(uint8_t num, int8_t var_05);
    };
#pragma pack(pop)
}
