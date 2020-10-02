#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

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

    namespace SpriteIndex
    {
        constexpr uint8_t null = 0xFF;
        constexpr uint8_t flag_unk7 = (1 << 7); // Set on electric multiple unit
    }

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
        uint8_t pad_01;
        uint8_t front_bogie_sprite_ind; // 0x02 index of bogie_sprites struct
        uint8_t back_bogie_sprite_ind;  // 0x03 index of bogie_sprites struct
        uint8_t body_sprite_ind;        // 0x04 index of a sprites struct
        uint8_t var_05;
    };

    struct vehicle_object_bogie_sprite
    {
        uint8_t pad_00[0x02 - 0x00];
        uint8_t var_02;
        uint8_t var_03;
        uint8_t var_04;
        uint8_t pad_05[0x12 - 0x5];
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
        uint8_t var_08;
        uint8_t var_09;
        uint8_t var_0A;
        uint8_t var_0B;
        uint8_t var_0C;
        uint8_t pad_0D;
        uint8_t sprite_num; // 0x0E
        uint8_t pad_0F[0x1E - 0xF];
    };

    namespace FlagsE0
    {
        constexpr uint16_t flag_02 = 1 << 2; // rollable? APT Passenger carriage
        constexpr uint16_t flag_03 = 1 << 3; // rollable? APT Driving carriage
        constexpr uint16_t rack_rail = 1 << 6;
        constexpr uint16_t unk_09 = 1 << 9; //anytrack??
        constexpr uint16_t can_couple = 1 << 11;
        constexpr uint16_t unk_12 = 1 << 12; //dualhead??
        constexpr uint16_t refittable = 1 << 14;
        constexpr uint16_t unk_15 = 1 << 15; //noannounce??
    }

    struct vehicle_object
    {
        string_id name;     // 0x00
        TransportMode mode; // 0x02
        VehicleType type;   // 0x03
        uint8_t var_04;
        uint8_t track_type;               // 0x05
        uint8_t num_mods;                 // 0x06
        uint8_t cost_index;               // 0x07
        int16_t cost_factor;              // 0x08
        uint8_t reliability;              // 0x0A
        uint8_t run_cost_index;           // 0x0B
        int16_t run_cost_factor;          // 0x0C
        uint8_t colour_type;              // 0x0E
        uint8_t num_compat;               // 0x0F
        uint16_t compatible_vehicles[8];  // 0x10 array of compatible vehicle_types
        uint8_t required_track_extras[4]; // 0x20
        vehicle_object_unk var_24[4];
        vehicle_object_sprite sprites[4];             // 0x3C
        vehicle_object_bogie_sprite bogie_sprites[2]; // 0xB4
        uint16_t power;                               // 0xD8
        uint16_t speed;                               // 0xDA
        uint16_t rack_speed;                          // 0xDC
        uint16_t weight;                              // 0xDE
        uint16_t flags;                               // 0xE0
        uint8_t max_primary_cargo;                    // 0xE2
        uint8_t max_secondary_cargo;                  // 0xE3
        uint32_t primary_cargo_types;                 // 0xE4
        uint32_t secondary_cargo_types;               // 0xE8
        uint8_t pad_EC[0x10C - 0xEC];
        uint8_t num_simultaneous_cargo_types; // 0x10C
        simple_animation animation[2];        // 0x10D
        uint8_t var_113;
        uint16_t designed;      // 0x114
        uint16_t obsolete;      // 0x116
        uint8_t rack_rail_type; // 0x118
        uint8_t startsnd_type;  // 0x119
        union
        {
            vehicle_object_sound_1 type_1;
            vehicle_object_sound_2 type_2;
            vehicle_object_sound_3 type_3;
        } sound;
        uint8_t pad_135[0x15A - 0x135];
        uint8_t num_sounds;             // 0x15A  possibly something else stuffed in (1<<7)
        uint8_t var_15B[0x15E - 0x15B]; // sound array size num_sounds/tbc??

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y);
        void getCargoString(char* buffer);
    };
#pragma pack(pop)
    static_assert(sizeof(vehicle_object) == 0x15E);

}
