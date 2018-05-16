#pragma once

#include "../objects/vehicle_object.h"
#include "thing.h"

namespace openloco
{
    namespace flags_5f
    {
        constexpr uint8_t breakdown_pending = 1 << 1;
        constexpr uint8_t broken_down = 1 << 2;
    }

    namespace flags_73
    {
        constexpr uint8_t broken_down = 1 << 0;
        constexpr uint8_t unk_2 = 1 << 1;
    }

    enum class vehicle_thing_type : uint8_t
    {
        vehicle_0 = 0,
        vehicle_1,
        vehicle_2,
        vehicle_bogie,
        vehicle_body_end,
        vehicle_body_cont,
        vehicle_6
    };

    struct vehicle;
    struct vehicle_0;
    struct vehicle_1;
    struct vehicle_2;
    struct vehicle_26;
    struct vehicle_bogie;
    struct vehicle_body;
    struct vehicle_6;

#pragma pack(push, 1)
    struct vehicle_base : thing_base
    {
        vehicle_thing_type type;
        uint8_t pad_02;
        uint8_t pad_03;
        thing_id_t next_thing_id; // 0x04
        uint8_t pad_06[0x09 - 0x06];
        uint8_t var_09;
        thing_id_t id;
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
        template<typename TType, vehicle_thing_type TClass>
        TType* as() const
        {
            return type == TClass ? (TType*)this : nullptr;
        }
    public:
        vehicle_bogie* as_vehicle_bogie() const { return as<vehicle_bogie, vehicle_thing_type::vehicle_bogie>(); }
        vehicle_body* as_vehicle_body() const
        {
            auto vehicle = as<vehicle_body, vehicle_thing_type::vehicle_body_end>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_body, vehicle_thing_type::vehicle_body_cont>();
        }
        vehicle_0* as_vehicle_0() const { return as<vehicle_0, vehicle_thing_type::vehicle_0>(); }
        vehicle_1* as_vehicle_1() const { return as<vehicle_1, vehicle_thing_type::vehicle_1>(); }
        vehicle_2* as_vehicle_2() const { return as<vehicle_2, vehicle_thing_type::vehicle_2>(); }
        vehicle_6* as_vehicle_6() const { return as<vehicle_6, vehicle_thing_type::vehicle_6>(); }
        vehicle_26* as_vehicle_2or6() const {
            auto vehicle = as<vehicle_26, vehicle_thing_type::vehicle_2>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_26, vehicle_thing_type::vehicle_6>();
        }

        vehicle* next_vehicle();
        vehicle* next_car();
    };

    struct vehicle : vehicle_base
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t var_28;
        uint8_t pad_2A[0x2C - 0x2A];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t pad_35;
        uint16_t var_36;
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
        uint8_t pad_56[0x5C - 0x56];
        uint8_t var_5C;
        uint8_t var_5D;
        uint8_t var_5E;
        uint8_t var_5F; // 0x5F (bit 1 = can break down)
        uint8_t pad_60[0x6A - 0x60];
        uint8_t var_6A;
        uint8_t pad_6B[0x6E - 0x6B];
        uint8_t var_6E;
        uint8_t pad_6F[0x73 - 0x6F];
        uint8_t var_73; // 0x73 (bit 0 = broken down)

        vehicle* next_vehicle();
        vehicle* next_car();
        vehicle_object* object() const;

        void update_head();
        


    private:
        bool update();




    };

    struct vehicle_0 : vehicle_base {
        uint8_t pad_20[0x30 - 0x20];
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t pad_35[0x36 - 0x35];
        uint16_t var_36;
        uint8_t var_38;
        uint8_t pad_39[0x42 - 0x39];
        uint8_t var_42;
        uint8_t pad_43[0x54 - 0x42];
        uint16_t station_object_id; // 0x54
        uint8_t pad_56[0x5C - 0x56];
        uint8_t var_5C;
        uint8_t var_5D;
        uint8_t var_5E;
        uint8_t pad_5F[0x6E - 0x5F];
        uint8_t var_6E;
        bool Update();
        void sub_4A8882();
        void sub_4A88A6(vehicle_26 * vehType2or6);
        void sub_4A8B7C(vehicle_26 * vehType2or6);
        void sub_4A88F7(vehicle_26 * vehType2or6, vehicle_object_sound_1 * buffer);
        void sub_4A8937(vehicle_26 * vehType2or6, vehicle_object_sound_2 * buffer);
        void sub_4A8A39(vehicle_26 * vehType2or6, vehicle_object_sound_3 * buffer);
        bool sub_4A8C81();
        bool sub_4A8D48();
        void sub_4BA8D4();
        bool sub_4A8CB6();
        void sub_4AF06E();
        void sub_4AD93A();
        void sub_4BAA76();
        void sub_4B9A2A();
        void sub_4B980A();
        void sub_4AA625();
        int16_t sub_4BABAD();
    };

    struct vehicle_1 : vehicle_base {
        bool Update();
    };

    struct vehicle_2 : vehicle_base {
        uint8_t pad_20[0x3A - 0x20];
        thing_id_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x44 - 0x3C];
        uint8_t var_44;
        uint8_t var_45;
        uint16_t var_46;
        uint16_t object_id; // 0x48
        uint8_t pad_4A[0x56 - 0x4A];
        uint32_t var_56;
        uint8_t var_5A;
        uint8_t pad_5B[0x73 - 0x5B];
        uint8_t var_73;

        thing * next_car();

        vehicle_object* object() const;
    };

    // TODO: Find out the difference between type 2 and 6
    struct vehicle_26 : vehicle_base {
        uint8_t pad_20[0x3A - 0x20];
        thing_id_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x44 - 0x3C];
        uint8_t var_44;
        uint8_t var_45;
        uint16_t var_46;
        uint16_t object_id; // 0x48

        thing * next_car();
        vehicle_object* object() const;
    };

    struct vehicle_bogie : vehicle_base {
        uint8_t pad_20[0x2C - 0x20];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t pad_35[0x5F - 0x35];
        uint8_t var_5F; // 0x5F (bit 1 = can break down)
    };

    struct vehicle_body : vehicle_base {
        uint8_t pad_20[0x38 - 0x20];
        uint8_t var_38;
        uint8_t object_sprite_type; // 0x39
        uint8_t pad_3A[0x40 - 0x39];
        uint16_t object_id; // 0x40
        uint8_t var_42;
        uint8_t pad_43;
        int16_t var_44;
        uint8_t var_46;
        uint8_t pad_47[0x54 - 0x47];
        uint8_t var_54;
        int8_t var_55;
        uint8_t pad_56[0x5E - 0x56];
        uint8_t var_5E;

        bool Update();

        void secondary_animation_update();
    private:
        void animation_update();
        void sub_4AC255(vehicle_bogie* back_bogie, vehicle_bogie* front_bogie);
        void sub_4AAB0B();
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
        uint16_t sub_4BE368(uint32_t distance);

        vehicle_object* object() const;
    };

    struct vehicle_6 : vehicle_base {
        uint8_t pad_20[0x4F - 0x20];
        uint16_t var_4F;
        uint8_t pad_51[0x5F - 0x51];
        uint8_t var_5F;
    };

#pragma pack(pop)
}
