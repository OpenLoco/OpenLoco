#pragma once

#include "Localisation/StringIds.h"
#include "Object.h"
#include "Speed.hpp"
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
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
    struct VehicleObjectFrictionSound
    {
        uint8_t soundObjectId;     // 0x0
        Speed32 minSpeed;          // 0x1 below this speed no sound created
        uint8_t speedFreqFactor;   // 0x5
        uint16_t baseFrequency;    // 0x6
        uint8_t speedVolumeFactor; // 0x8
        uint8_t baseVolume;        // 0x9
        uint8_t maxVolume;         // 0xA
    };

    struct VehicleObjectEngine1Sound
    {
        uint8_t soundObjectId;     // 0x0
        uint16_t defaultFrequency; // 0x1
        uint8_t defaultVolume;     // 0x3
        uint16_t var_04;
        uint8_t var_06;
        uint16_t var_07;
        uint8_t var_09;
        uint16_t freqIncreaseStep;  // 0xA
        uint16_t freqDecreaseStep;  // 0xC
        uint8_t volumeIncreaseStep; // 0xE
        uint8_t volumeDecreaseStep; // 0xF
        uint8_t speedFreqFactor;    // 0x10
    };

    struct VehicleObjectEngine2Sound
    {
        uint8_t soundObjectId;         // 0x0
        uint16_t defaultFrequency;     // 0x1
        uint8_t defaultVolume;         // 0x2
        uint16_t firstGearFrequency;   // 0x4 All subsequent gears are based on this frequency
        Speed16 firstGearSpeed;        // 0x6
        uint16_t secondGearFreqFactor; // 0x8
        Speed16 secondGearSpeed;       // 0xA
        uint16_t thirdGearFreqFactor;  // 0xC
        Speed16 thirdGearSpeed;        // 0xE
        uint16_t fourthGearFreqFactor; // 0x10
        uint8_t var_12;
        uint8_t var_13;
        uint16_t freqIncreaseStep;  // 0x14
        uint16_t freqDecreaseStep;  // 0x16
        uint8_t volumeIncreaseStep; // 0x18
        uint8_t volumeDecreaseStep; // 0x19
        uint8_t speedFreqFactor;    // 0x1A
    };

    struct simple_animation
    {
        uint8_t objectId;           // 0x00 (object loader fills this in)
        uint8_t height;             // 0x01
        simple_animation_type type; // 0x02
    };

    struct vehicle_object_unk
    {
        uint8_t length; // 0x00
        uint8_t var_01;
        uint8_t frontBogieSpriteInd; // 0x02 index of bogieSprites struct
        uint8_t backBogieSpriteInd;  // 0x03 index of bogieSprites struct
        uint8_t bodySpriteInd;       // 0x04 index of a bodySprites struct
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
        uint8_t numCargoLoadFrames;      // 0x03
        uint8_t numCargoFrames;          // 0x04
        uint8_t numRollFrames;           // 0x05
        uint8_t bogeyPosition;           // 0x06
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
        constexpr uint16_t rackRail = 1 << 6;
        constexpr uint16_t unk_08 = 1 << 8;
        constexpr uint16_t unk_09 = 1 << 9; // anytrack??
        constexpr uint16_t speedControl = 1 << 10;
        constexpr uint16_t canCouple = 1 << 11;
        constexpr uint16_t unk_12 = 1 << 12; // dualhead??
        constexpr uint16_t isHelicopter = 1 << 13;
        constexpr uint16_t refittable = 1 << 14;
        constexpr uint16_t unk_15 = 1 << 15; // noannounce??
    }

    enum class DrivingSoundType : uint8_t
    {
        none,
        friction,
        engine1,
        engine2
    };

    namespace NumStartSounds
    {
        constexpr uint8_t hasCrossingWhistle = 1 << 7;
        constexpr uint8_t mask = 0x7F;
    }

    struct VehicleObject
    {
        static constexpr auto kObjectType = ObjectType::vehicle;
        static constexpr auto kMaxBodySprites = 4;

        string_id name;     // 0x00
        TransportMode mode; // 0x02
        VehicleType type;   // 0x03
        uint8_t var_04;
        uint8_t trackType;              // 0x05
        uint8_t numMods;                // 0x06
        uint8_t costIndex;              // 0x07
        int16_t costFactor;             // 0x08
        uint8_t reliability;            // 0x0A
        uint8_t runCostIndex;           // 0x0B
        int16_t runCostFactor;          // 0x0C
        uint8_t colourType;             // 0x0E
        uint8_t numCompat;              // 0x0F
        uint16_t compatibleVehicles[8]; // 0x10 array of compatible vehicle_types
        uint8_t requiredTrackExtras[4]; // 0x20
        vehicle_object_unk var_24[4];
        VehicleObjectBodySprite bodySprites[kMaxBodySprites]; // 0x3C
        vehicle_object_bogie_sprite bogieSprites[2];          // 0xB4
        uint16_t power;                                       // 0xD8
        Speed16 speed;                                        // 0xDA
        Speed16 rackSpeed;                                    // 0xDC
        uint16_t weight;                                      // 0xDE
        uint16_t flags;                                       // 0xE0
        uint8_t maxPrimaryCargo;                              // 0xE2 size is relative to the first primaryCargoTypes
        uint8_t maxSecondaryCargo;                            // 0xE3
        uint32_t primaryCargoTypes;                           // 0xE4
        uint32_t secondaryCargoTypes;                         // 0xE8
        uint8_t cargoTypeSpriteOffsets[32];                   // 0xEC
        uint8_t numSimultaneousCargoTypes;                    // 0x10C
        simple_animation animation[2];                        // 0x10D
        uint8_t var_113;
        uint16_t designed;                 // 0x114
        uint16_t obsolete;                 // 0x116
        uint8_t rackRailType;              // 0x118
        DrivingSoundType drivingSoundType; // 0x119
        union
        {
            VehicleObjectFrictionSound friction;
            VehicleObjectEngine1Sound engine1;
            VehicleObjectEngine2Sound engine2;
        } sound;
        uint8_t pad_135[0x15A - 0x135];
        uint8_t numStartSounds;         // 0x15A use mask when accessing hasCrossingWhistle stuffed in (1 << 7)
        SoundObjectId_t startSounds[3]; // 0x15B sound array length numStartSounds highest sound is the crossing whistle

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, const int16_t width) const;
        void getCargoString(char* buffer) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data);
        void unload();
        uint32_t getLength() const;
    };
#pragma pack(pop)
    static_assert(sizeof(VehicleObject) == 0x15E);

    namespace StringIds
    {
        constexpr string_id getVehicleType(const VehicleType type)
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
