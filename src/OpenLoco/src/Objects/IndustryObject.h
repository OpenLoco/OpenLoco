#pragma once

#include "BuildingCommon.h"
#include "Object.h"
#include "Types.hpp"
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
        class DrawingContext;
    }

    enum class IndustryObjectFlags : uint32_t
    {
        none = 0U,
        builtInClusters = 1U << 0,
        builtOnHighGround = 1U << 1,
        builtOnLowGround = 1U << 2,
        builtOnSnow = 1U << 3,        // above summer snow line
        builtBelowSnowLine = 1U << 4, // below winter snow line
        builtOnFlatGround = 1U << 5,
        builtNearWater = 1U << 6,
        builtAwayFromWater = 1U << 7,
        builtOnWater = 1U << 8,
        builtNearTown = 1U << 9,
        builtAwayFromTown = 1U << 10,
        builtNearTrees = 1U << 11,
        builtRequiresOpenSpace = 1U << 12,
        oilfield = 1U << 13,
        mines = 1U << 14,
        notRotatable = 1U << 15,
        canBeFoundedByPlayer = 1U << 16,
        requiresAllCargo = 1U << 17,
        unk18 = 1U << 18,
        unk19 = 1U << 19,
        hasShadows = 1U << 21,
        unk23 = 1U << 23,
        builtInDesert = 1U << 24,
        builtNearDesert = 1U << 25,
        unk26 = 1U << 26,
        unk27 = 1U << 27,
        flag_28 = 1U << 28,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(IndustryObjectFlags);

#pragma pack(push, 1)
    struct IndustryObjectUnk38
    {
        uint8_t var_00;
        uint8_t var_01;
    };
    static_assert(sizeof(IndustryObjectUnk38) == 0x2);

    struct IndustryObjectProductionRateRange
    {
        uint16_t min;
        uint16_t max;
    };
    static_assert(sizeof(IndustryObjectProductionRateRange) == 0x4);

    struct IndustryObject
    {
        static constexpr auto kObjectType = ObjectType::industry;

        StringId name;               // 0x0
        StringId var_02;             // 0x2
        StringId nameClosingDown;    // 0x4
        StringId nameUpProduction;   // 0x6
        StringId nameDownProduction; // 0x8
        StringId nameSingular;       // 0x0A
        StringId namePlural;         // 0x0C
        uint32_t var_0E;             // 0x0E shadows image id base
        uint32_t var_12;             // 0x12 Base image id for building 0
        uint32_t var_16;
        uint32_t var_1A;
        uint8_t numBuildingParts;                            // 0x1E
        uint8_t numBuildingVariations;                       // 0x1F
        const uint8_t* buildingPartHeights;                  // 0x20 This is the height of a building image
        const BuildingPartAnimation* buildingPartAnimations; // 0x24
        const uint8_t* animationSequences[4];                // 0x28 Access with getAnimationSequence helper method
        const IndustryObjectUnk38* var_38;                   // 0x38 Access with getUnk38 helper method
        const uint8_t* buildingVariationParts[32];           // 0x3C Access with getBuildingParts helper method
        uint8_t minNumBuildings;                             // 0xBC
        uint8_t maxNumBuildings;                             // 0xBD
        const uint8_t* buildings;                            // 0xBE
        uint32_t availableColours;                           // 0xC2 bitset
        uint32_t buildingSizeFlags;                          // 0xC6 flags indicating the building types size 1:large4x4, 0:small1x1
        uint16_t designedYear;                               // 0xCA start year
        uint16_t obsoleteYear;                               // 0xCC end year
        // Total industries of this type that can be created in a scenario
        // Note: this is not directly comparable to total industries and varies based
        // on scenario total industries cap settings. At low industries cap this value is ~3x the
        // amount of industries in a scenario.
        uint8_t totalOfTypeInScenario;                              // 0xCE
        uint8_t costIndex;                                          // 0xCF
        int16_t costFactor;                                         // 0xD0
        int16_t clearCostFactor;                                    // 0xD2
        uint8_t scaffoldingSegmentType;                             // 0xD4
        Colour scaffoldingColour;                                   // 0xD5
        IndustryObjectProductionRateRange initialProductionRate[2]; // 0xD6
        uint8_t producedCargoType[2];                               // 0xDE (0xFF = null)
        uint8_t requiredCargoType[3];                               // 0xE0 (0xFF = null)
        Colour mapColour;                                           // 0xE3
        IndustryObjectFlags flags;                                  // 0xE4
        uint8_t var_E8;
        uint8_t var_E9;
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t var_EC;       // Used by Livestock cow shed count??
        uint8_t wallTypes[4]; // 0xED There can be up to 4 different wall types for an industry
        // Selection of wall types isn't completely random from the 4 it is biased into 2 groups of 2 (wall and entrance)
        uint8_t buildingWall;         // 0xF1
        uint8_t buildingWallEntrance; // 0xF2 An alternative wall type that looks like a gate placed at random places in building perimeter
        uint8_t var_F3;

        bool requiresCargo() const;
        bool producesCargo() const;
        char* getProducedCargoString(const char* buffer) const;
        char* getRequiredCargoString(const char* buffer) const;
        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        void drawIndustry(Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
        std::span<const std::uint8_t> getBuildingParts(const uint8_t buildingType) const;
        std::span<const std::uint8_t> getAnimationSequence(const uint8_t unk) const;
        std::span<const IndustryObjectUnk38> getUnk38() const;

        constexpr bool hasFlags(IndustryObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != IndustryObjectFlags::none;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(IndustryObject) == 0xF4);
}
