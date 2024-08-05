#pragma once

#include "BuildingCommon.h"
#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
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

    enum class DockObjectFlags : uint16_t
    {
        none = 0,
        hasShadows = 1U << 0,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(DockObjectFlags);

#pragma pack(push, 1)
    struct DockObject
    {
        static constexpr auto kObjectType = ObjectType::dock;

        StringId name;
        int16_t buildCostFactor; // 0x02
        int16_t sellCostFactor;  // 0x04
        uint8_t costIndex;       // 0x06
        uint8_t var_07;
        uint32_t image;                                      // 0x08
        uint32_t buildingImage;                              // 0x0C
        DockObjectFlags flags;                               // 0x10
        uint8_t numBuildingParts;                            // 0x12
        uint8_t numBuildingVariations;                       // 0x13 must be 1 or 0
        const uint8_t* partHeights;                          // 0x14
        const BuildingPartAnimation* buildingPartAnimations; // 0x18
        const uint8_t* buildingVariationParts[1];            // 0x1C odd that this is size 1 but that is how its used
        uint16_t designedYear;                               // 0x20
        uint16_t obsoleteYear;                               // 0x22
        World::Pos2 boatPosition;                            // 0x24

        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();

        std::span<const std::uint8_t> getBuildingParts(const uint8_t buildingType) const;

        constexpr bool hasFlags(DockObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != DockObjectFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(DockObject) == 0x28);
}
