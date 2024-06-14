#pragma once

#include "BuildingCommon.h"
#include "Graphics/Colour.h"
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
        struct RenderTarget;
    }

    enum class BuildingObjectFlags : uint8_t
    {
        none = 0U,
        largeTile = 1U << 0, // 2x2 tile
        miscBuilding = 1U << 1,
        undestructible = 1U << 2,
        isHeadquarters = 1U << 3,
        hasShadows = 1U << 4,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BuildingObjectFlags);

#pragma pack(push, 1)
    // Todo this is the same as industry obj
    struct PartAnimation
    {
        uint8_t numFrames;      // 0x0 Must be a power of 2 (0 = no part animation, could still have animation sequence)
        uint8_t animationSpeed; // 0x1 Also encodes in bit 7 if the animation is position modified
    };
    static_assert(sizeof(PartAnimation) == 0x2);

    struct BuildingObject
    {
        static constexpr auto kObjectType = ObjectType::building;

        StringId name;                               // 0x0
        uint32_t image;                              // 0x2
        uint8_t numParts;                            // 0x6
        uint8_t numVariations;                       // 0x7
        const uint8_t* partHeights;                  // 0x8
        const BuildingPartAnimation* partAnimations; // 0xC
        const uint8_t* variationParts[32];           // 0x10 Access with getBuildingParts helper method
        uint32_t colours;                            // 0x90
        uint16_t designedYear;                       // 0x94
        uint16_t obsoleteYear;                       // 0x96
        BuildingObjectFlags flags;                   // 0x98
        uint8_t clearCostIndex;                      // 0x99
        uint16_t clearCostFactor;                    // 0x9A
        uint8_t scaffoldingSegmentType;              // 0x9C
        Colour scaffoldingColour;                    // 0x9D
        uint8_t generatorFunction;                   // 0x9E
        uint8_t averageNumberOnMap;                  // 0x9F
        uint8_t producedQuantity[2];                 // 0xA0
        uint8_t producedCargoType[2];                // 0xA2
        uint8_t requiredCargoType[2];                // 0xA4
        uint8_t var_A6[2];                           // 0xA6
        uint8_t var_A8[2];                           // 0xA8
        int16_t demolishRatingReduction;             // 0XAA
        uint8_t var_AC;                              // 0xAC
        uint8_t numElevatorSequences;                // 0XAD
        const uint8_t* elevatorHeightSequences[4];   // 0XAE Access with getElevatorHeightSequence helper method

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawBuilding(Gfx::RenderTarget* clipped, uint8_t buildingRotation, int16_t x, int16_t y, Colour colour) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        std::span<const std::uint8_t> getBuildingParts(const uint8_t variation) const;
        std::span<const std::uint8_t> getElevatorHeightSequence(const uint8_t animIdx) const;

        constexpr bool hasFlags(BuildingObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != BuildingObjectFlags::none;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(BuildingObject) == 0xBE);
}
