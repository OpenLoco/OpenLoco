#pragma once

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

    enum class LandObjectFlags : uint8_t
    {
        none = 0U,
        unk0 = 1U << 0,
        hasReplacementLandHeader = 1U << 1,
        isDesert = 1U << 2,
        noTrees = 1U << 3,
        unk4 = 1U << 4,
        unk5 = 1U << 5,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(LandObjectFlags);

#pragma pack(push, 1)
    struct LandObject
    {
        static constexpr auto kObjectType = ObjectType::land;

        StringId name;
        uint8_t costIndex;                // 0x02
        uint8_t numGrowthStages;          // 0x03 "Healing" animation after disturbing land
        uint8_t numImageAngles;           // 0x04 How many rotation angles the land has
        LandObjectFlags flags;            // 0x05
        uint8_t cliffEdgeHeader;          // 0x06
        uint8_t replacementLandHeader;    // 0x07
        int16_t costFactor;               // 0x08
        uint32_t image;                   // 0x0A calculated value
        uint32_t numImagesPerGrowthStage; // 0x0E calculated value
        uint32_t cliffEdgeImage;          // 0x12 calculated value
        uint32_t mapPixelImage;           // 0x16 calculated value
        uint8_t distributionPattern;      // 0x1A
        uint8_t numVariations;            // 0x1B
        uint8_t variationLikelihood;      // 0x1C
        uint8_t pad_1D;

        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;

        constexpr bool hasFlags(LandObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != LandObjectFlags::none;
        }
    };
    static_assert(sizeof(LandObject) == 0x1E);
#pragma pack(pop)

    namespace Land::ImageIds
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
