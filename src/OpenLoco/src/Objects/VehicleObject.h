#pragma once

#include "Localisation/StringIds.h"
#include "Object.h"
#include "Speed.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <span>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
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

    enum class SimpleAnimationType : uint8_t
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
    static_assert(sizeof(VehicleObjectFrictionSound) == 0xB);

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
    static_assert(sizeof(VehicleObjectEngine1Sound) == 0x11);

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
    static_assert(sizeof(VehicleObjectEngine2Sound) == 0x1B);

    struct VehicleObjectSimpleAnimation
    {
        uint8_t objectId;         // 0x00 (object loader fills this in)
        uint8_t height;           // 0x01
        SimpleAnimationType type; // 0x02
    };
    static_assert(sizeof(VehicleObjectSimpleAnimation) == 0x3);

    struct VehicleObjectUnk
    {
        uint8_t length; // 0x00
        uint8_t var_01;
        uint8_t frontBogieSpriteInd; // 0x02 index of bogieSprites struct
        uint8_t backBogieSpriteInd;  // 0x03 index of bogieSprites struct
        uint8_t bodySpriteInd;       // 0x04 index of a bodySprites struct
        uint8_t var_05;
    };
    static_assert(sizeof(VehicleObjectUnk) == 0x6);

    enum class BogieSpriteFlags : uint8_t
    {
        none = 0U,
        hasSprites = 1U << 0,         // If not set then no bogie will be loaded
        rotationalSymmetry = 1U << 1, // requires 16 rather than 32 sprites
        hasGentleSprites = 1U << 2,   // for gentle slopes
        hasSteepSprites = 1U << 3,    // for steep slopes
        unk_4 = 1U << 4,              // Increases bounding box size
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BogieSpriteFlags);

    struct VehicleObjectBogieSprite
    {
        uint8_t rollStates;      // 0x0 valid values 1, 2, 4 related to bogie->var_46 (identical in value to numRollSprites)
        BogieSpriteFlags flags;  // 0x1 BogieSpriteFlags
        uint8_t width;           // 0x2 sprite width
        uint8_t heightNegative;  // 0x3 sprite height negative
        uint8_t heightPositive;  // 0x4 sprite height positive
        uint8_t numRollSprites;  // 0x5
        uint32_t flatImageIds;   // 0x6 flat sprites
        uint32_t gentleImageIds; // 0xA gentle sprites
        uint32_t steepImageIds;  // 0xE steep sprites

        constexpr bool hasFlags(BogieSpriteFlags flagsToTest) const
        {
            return (flags & flagsToTest) != BogieSpriteFlags::none;
        }
    };
    static_assert(sizeof(VehicleObjectBogieSprite) == 0x12);

    enum class BodySpriteFlags : uint8_t
    {
        none = 0U,
        hasSprites = 1U << 0,         // If not set then no body will be loaded
        rotationalSymmetry = 1U << 1, // requires 32 rather than 64 sprites
        hasUnkSprites = 1U << 2,
        hasGentleSprites = 1U << 3, // for gentle slopes
        hasSteepSprites = 1U << 4,  // for steep slopes
        hasBrakingLights = 1U << 5,
        hasSpeedAnimation = 1U << 6, // Speed based animation (such as hydrofoil)
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BodySpriteFlags);

    struct VehicleObjectBodySprite
    {
        uint8_t numFlatRotationFrames;   // 0x00 4, 8, 16, 32, 64?
        uint8_t numSlopedRotationFrames; // 0x01 4, 8, 16, 32?
        uint8_t numAnimationFrames;      // 0x02
        uint8_t numCargoLoadFrames;      // 0x03
        uint8_t numCargoFrames;          // 0x04
        uint8_t numRollFrames;           // 0x05
        uint8_t bogeyPosition;           // 0x06
        BodySpriteFlags flags;           // 0x07
        uint8_t width;                   // 0x08 sprite width
        uint8_t heightNegative;          // 0x09 sprite height negative
        uint8_t heightPositive;          // 0x0A sprite height positive
        uint8_t flatYawAccuracy;         // 0x0B 0 - 4 accuracy of yaw on flat built from numFlatRotationFrames (0 = lowest accuracy 3bits, 4 = highest accuracy 7bits)
        uint8_t slopedYawAccuracy;       // 0x0C 0 - 3 accuracy of yaw on slopes built from numSlopedRotationFrames  (0 = lowest accuracy 3bits, 3 = highest accuracy 6bits)
        uint8_t numFramesPerRotation;    // 0x0D numAnimationFrames * numCargoFrames * numRollFrames + 1 (for braking lights)
        uint32_t flatImageId;            // 0x0E
        uint32_t unkImageId;             // 0x12
        uint32_t gentleImageId;          // 0x16
        uint32_t steepImageId;           // 0x1A

        constexpr bool hasFlags(BodySpriteFlags flagsToTest) const
        {
            return (flags & flagsToTest) != BodySpriteFlags::none;
        }
    };

    enum class VehicleObjectFlags : uint16_t
    {
        none = 0U,
        flag_02 = 1U << 2, // rollable? APT Passenger carriage
        flag_03 = 1U << 3, // rollable? APT Driving carriage
        rackRail = 1U << 6,
        unk_08 = 1U << 8,
        unk_09 = 1U << 9, // anytrack??
        speedControl = 1U << 10,
        canCouple = 1U << 11,
        unk_12 = 1U << 12, // dualhead??
        isHelicopter = 1U << 13,
        refittable = 1U << 14,
        unk_15 = 1U << 15, // noannounce??
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(VehicleObjectFlags);

    enum class DrivingSoundType : uint8_t
    {
        none,
        friction,
        engine1,
        engine2
    };

    namespace NumStartSounds
    {
        constexpr uint8_t kHasCrossingWhistle = 1 << 7;
        constexpr uint8_t kMask = 0x7F;
    }

    struct VehicleObject
    {
        static constexpr auto kObjectType = ObjectType::vehicle;
        static constexpr auto kMaxBodySprites = 4;

        StringId name;      // 0x00
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
        VehicleObjectUnk var_24[4];
        VehicleObjectBodySprite bodySprites[kMaxBodySprites]; // 0x3C
        VehicleObjectBogieSprite bogieSprites[2];             // 0xB4
        uint16_t power;                                       // 0xD8
        Speed16 speed;                                        // 0xDA
        Speed16 rackSpeed;                                    // 0xDC
        uint16_t weight;                                      // 0xDE
        VehicleObjectFlags flags;                             // 0xE0
        uint8_t maxCargo[2];                                  // 0xE2 size is relative to the first cargoTypes
        uint32_t cargoTypes[2];                               // 0xE4
        uint8_t cargoTypeSpriteOffsets[32];                   // 0xEC
        uint8_t numSimultaneousCargoTypes;                    // 0x10C
        VehicleObjectSimpleAnimation animation[2];            // 0x10D
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
        uint8_t numStartSounds;         // 0x15A use mask when accessing kHasCrossingWhistle stuffed in (1 << 7)
        SoundObjectId_t startSounds[3]; // 0x15B sound array length numStartSounds highest sound is the crossing whistle

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, const int16_t width) const;
        void getCargoString(char* buffer) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
        uint32_t getLength() const;
        constexpr bool hasFlags(VehicleObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != VehicleObjectFlags::none;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(VehicleObject) == 0x15E);

    namespace StringIds
    {
        constexpr StringId getVehicleType(const VehicleType type)
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
