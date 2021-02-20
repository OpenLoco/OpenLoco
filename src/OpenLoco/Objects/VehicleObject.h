#pragma once

#include "../Localisation/StringIds.h"
#include "../Localisation/StringManager.h"
#include "../Speed.hpp"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

    enum class TransportMode : uint8_t
    {
        rail = 0,
        road = 1,
        air = 2,
        water = 3
    };

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
        uint8_t var_01;
        uint8_t front_bogie_sprite_ind; // 0x02 index of bogie_sprites struct
        uint8_t back_bogie_sprite_ind;  // 0x03 index of bogie_sprites struct
        uint8_t body_sprite_ind;        // 0x04 index of a bodySprites struct
        uint8_t var_05;
    };

    namespace BogieSpriteFlags
    {
        constexpr uint8_t hasSprites = (1 << 0);         // If not set then no bogie will be loaded
        constexpr uint8_t rotationalSymmetry = (1 << 1); // requires 16 rather than 32 sprites
        constexpr uint8_t hasGentleSprites = (1 << 2);   // for gentle slopes
        constexpr uint8_t hasSteepSprites = (1 << 3);    // for steep slopes
        constexpr uint8_t unk_4 = (1 << 4);              // Increases bounding box size
    }

    struct vehicle_object_bogie_sprite
    {
        uint8_t rollStates;      // 0x0 valid values 1, 2, 4 related to bogie->var_46 (identical in value to numRollSprites)
        uint8_t flags;           // 0x1 BogieSpriteFlags
        uint8_t var_02;          // sprite width
        uint8_t var_03;          // sprite height negative
        uint8_t var_04;          // sprite height positive
        uint8_t numRollSprites;  // 0x5
        uint32_t flatImageIds;   // 0x6 flat sprites
        uint32_t gentleImageIds; // 0xA gentle sprites
        uint32_t steepImageIds;  // 0xE steep sprites
    };

    static_assert(sizeof(vehicle_object_bogie_sprite) == 0x12);

    namespace BodySpriteFlags
    {
        constexpr uint8_t hasSprites = (1 << 0);         // If not set then no body will be loaded
        constexpr uint8_t rotationalSymmetry = (1 << 1); // requires 32 rather than 64 sprites
        constexpr uint8_t hasUnkSprites = (1 << 2);
        constexpr uint8_t hasGentleSprites = (1 << 3); // for gentle slopes
        constexpr uint8_t hasSteepSprites = (1 << 4);  // for steep slopes
        constexpr uint8_t hasBrakingLights = (1 << 5);
        constexpr uint8_t hasSpeedAnimation = (1 << 6); // Speed based animation (such as hydrofoil)
    }

    struct VehicleObjectBodySprite
    {
        uint8_t numFlatRotationFrames;   // 0x00 4, 8, 16, 32, 64?
        uint8_t numSlopedRotationFrames; // 0x01 4, 8, 16, 32?
        uint8_t numAnimationFrames;      // 0x02
        uint8_t vehicle_type;            // 0x03
        uint8_t numCargoFrames;          // 0x04
        uint8_t numRollFrames;           // 0x05
        uint8_t bogey_position;          // 0x06
        uint8_t flags;                   // 0x07
        uint8_t var_08;                  // sprite width
        uint8_t var_09;                  // sprite height negative
        uint8_t var_0A;                  // sprite height positive
        uint8_t var_0B;                  // 0 - 4 rotational sprites types on flat built from numFlatRotationFrames
        uint8_t var_0C;                  // 0 - 3 rotational sprites types on slopes built from numSlopedRotationFrames
        uint8_t numFramesPerRotation;    // 0x0D numAnimationFrames * numCargoFrames * numRollFrames + 1 (for braking lights)
        uint32_t flatImageId;            // 0x0E
        uint32_t unkImageId;             // 0x12
        uint32_t gentleImageId;          // 0x16
        uint32_t steepImageId;           // 0x1A
    };

    namespace FlagsE0
    {
        constexpr uint16_t flag_02 = 1 << 2; // rollable? APT Passenger carriage
        constexpr uint16_t flag_03 = 1 << 3; // rollable? APT Driving carriage
        constexpr uint16_t rack_rail = 1 << 6;
        constexpr uint16_t unk_09 = 1 << 9; //anytrack??
        constexpr uint16_t speed_control = 1 << 10;
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
        VehicleObjectBodySprite bodySprites[4];       // 0x3C
        vehicle_object_bogie_sprite bogie_sprites[2]; // 0xB4
        uint16_t power;                               // 0xD8
        Speed16 speed;                                // 0xDA
        uint16_t rack_speed;                          // 0xDC
        uint16_t weight;                              // 0xDE
        uint16_t flags;                               // 0xE0
        uint8_t max_primary_cargo;                    // 0xE2 size is relative to the first primary_cargo_types
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

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y, const int16_t width) const;
        void getCargoString(char* buffer) const;
    };
#pragma pack(pop)
    static_assert(sizeof(vehicle_object) == 0x15E);

    namespace StringIds
    {
        constexpr string_id getVehicleType(VehicleType type)
        {
            switch (type)
            {
                case VehicleType::train:
                    return StringIds::train;
                case VehicleType::bus:
                    return StringIds::bus;
                case VehicleType::truck:
                    return StringIds::truck;
                case VehicleType::tram:
                    return StringIds::tram;
                case VehicleType::aircraft:
                    return StringIds::aircraft;
                case VehicleType::ship:
                    return StringIds::ship;
            }
            return StringIds::empty;
        }
    }
}
