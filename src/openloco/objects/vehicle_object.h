#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    enum class TransportMode : uint8_t
    {
        rail = 0,
        road,
        air,
        water
    };

    enum class VehicleType : uint8_t
    {
        train = 0,
        bus,
        truck,
        tram,
        plane,
        ship
    };

    constexpr uint8_t vehicleTypeCount = 6;

    enum class simple_animation_type : uint8_t
    {
        none = 0,
        steam_puff1,
        steam_puff2,
        steam_puff3,
        diesel_exhaust1,
        electric_spark1,
        electric_spark2,
        diesel_exhaust2,
        ship_wake
    };
#pragma pack(push, 1)
    struct vehicle_object_sound_1
    {
        uint8_t sound_object_id;
        uint32_t var_01;
        uint8_t var_05;
        uint16_t var_06;
        uint8_t var_08;
        uint8_t var_09;
        uint8_t var_0A;
    };

    struct vehicle_object_sound_2
    {
        uint8_t sound_object_id;
        uint16_t var_01;
        uint8_t var_03;
        uint16_t var_04;
        uint8_t var_06;
        uint16_t var_07;
        uint8_t var_09;
        uint16_t var_0A;
        uint16_t var_0C;
        uint8_t var_0E;
        uint8_t var_0F;
        uint8_t var_10;
    };

    struct vehicle_object_sound_3
    {
        uint8_t sound_object_id;
        uint16_t var_01;
        uint8_t var_03;
        uint16_t var_04;
        uint16_t var_06;
        uint16_t var_08;
        uint16_t var_0A;
        uint16_t var_0C;
        uint16_t var_0E;
        uint16_t var_10;
        uint8_t var_12;
        uint8_t var_13;
        uint16_t var_14;
        uint16_t var_16;
        uint8_t var_18;
        uint8_t var_19;
        uint8_t var_1A;
    };

    struct simple_animation
    {
        uint8_t object_id;          // 0x00 (object loader fills this in)
        uint8_t height;             // 0x01
        simple_animation_type type; // 0x02
    };

    struct vehicle_object_unk
    {
        uint8_t length; // 0x00
        uint8_t pad_01[0x04 - 0x01];
        uint8_t sprite_ind; // 0x04
        uint8_t var_05;
    };

    struct vehicle_object_sprite
    {
        uint8_t num_dir; // 0x00
        uint8_t pad_01;
        int8_t var_02;
        uint8_t vehicle_type; // 0x03
        uint8_t num_units;    // 0x04
        uint8_t var_05;
        uint8_t bogey_position; // 0x06
        uint8_t flags;          // 0x07
        uint8_t pad_08[0x0B - 0x08];
        uint8_t var_0B;
        uint8_t var_0C;
        uint8_t pad_0D;
        uint8_t sprite_num; // 0x0E
        uint8_t pad_0F[0x1E - 0xF];
    };

    struct vehicle_object
    {
        string_id name;     // 0x00 probably not confirmed
        TransportMode mode; // 0x02
        VehicleType type;   // 0x03
        uint8_t pad_04[0x6 - 0x4];
        uint8_t num_mods;      // 0x06
        uint8_t cost_ind;      // 0x07
        int16_t cost_fact;     // 0x08
        uint8_t reliability;   // 0x0A
        uint8_t run_cost_ind;  // 0x0B
        int16_t run_cost_fact; // 0x0C
        uint8_t colour_type;   // 0x0E
        uint8_t num_compat;    // 0x0F
        uint8_t pad_10[0x24 - 0x10];
        vehicle_object_unk var_24[4];
        vehicle_object_sprite sprites[4]; // 0x3C
        uint8_t pad_B4[0xD8 - 0xB4];
        uint16_t power;      // 0xD8
        uint16_t speed;      // 0xDA
        uint16_t rack_speed; // 0xDC
        uint16_t weight;     // 0xDE
        uint16_t flags;      // 0xE0
        uint8_t pad_E2[0x10D - 0xE2];
        simple_animation animation[2]; // 0x10D
        uint8_t var_113;
        uint16_t designed; // 0x114
        uint16_t obsolete; // 0x116
        uint8_t pad_118;
        uint8_t startsnd_type; // 0x119
        union
        {
            vehicle_object_sound_1 type_1;
            vehicle_object_sound_2 type_2;
            vehicle_object_sound_3 type_3;
        } sound;
        uint8_t pad_135[0x15A - 0x135];
        uint8_t num_sounds;             // 0x15A  possibly something else stuffed in (1<<7)
        uint8_t var_15B[0x15E - 0x15B]; // sound array size num_sounds/tbc??
    };
#pragma pack(pop)
}
