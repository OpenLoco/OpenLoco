#pragma once

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
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BuildingObjectFlags);

#pragma pack(push, 1)
    struct BuildingObject
    {
        static constexpr auto kObjectType = ObjectType::building;

        string_id name;                     // 0x0
        uint32_t image;                     // 0x2
        uint8_t var_06;                     // 0x6
        uint8_t numVariations;              // 0x7
        const uint8_t* variationHeights;    // 0x8
        const uint16_t* var_0C;             // 0xC
        const uint8_t* variationsArr10[32]; // 0x10
        uint32_t colours;                   // 0x90
        uint16_t designedYear;              // 0x94
        uint16_t obsoleteYear;              // 0x96
        BuildingObjectFlags flags;          // 0x98
        uint8_t clearCostIndex;             // 0x99
        uint16_t clearCostFactor;           // 0x9A
        uint8_t scaffoldingSegmentType;     // 0x9C
        Colour scaffoldingColour;           // 0x9D
        uint8_t pad_9E[0xA0 - 0x9E];
        uint8_t producedQuantity[2];     // 0xA0
        uint8_t producedCargoType[2];    // 0xA2
        uint8_t var_A4[2];               // 0xA4 Some type of Cargo
        uint8_t var_A6[2];               // 0xA6
        uint8_t var_A8[2];               // 0xA8
        int16_t demolishRatingReduction; // 0XAA
        uint8_t var_AC;                  // 0xAC
        uint8_t var_AD;                  // 0XAD
        const uint8_t* var_AE[4];        // 0XAE ->0XB2->0XB6->0XBA->0XBE (4 byte pointers)

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawBuilding(Gfx::RenderTarget* clipped, uint8_t buildingRotation, int16_t x, int16_t y, Colour colour) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        constexpr bool hasFlags(BuildingObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != BuildingObjectFlags::none;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(BuildingObject) == 0xBE);
}
