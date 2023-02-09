#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

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

#pragma pack(push, 1)
    struct ScaffoldingObject
    {
        static constexpr auto kObjectType = ObjectType::scaffolding;

        string_id name;
        uint32_t image;             // 0x02
        uint16_t segmentHeights[3]; // 0x06
        uint16_t roofHeights[3];    // 0x0C

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        // 0x0042DF0B
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(ScaffoldingObject) == 0x12);

    namespace Scaffolding::ImageIds
    {
        constexpr uint32_t type01x1SegmentFlagsBack = 0;
        constexpr uint32_t type01x1SegmentFlagsFront = 1;
        constexpr uint32_t type01x1RoofNE = 2;
        constexpr uint32_t type01x1RoofSE = 3;
        constexpr uint32_t type01x1RoofSW = 4;
        constexpr uint32_t type01x1RoofNW = 5;
        constexpr uint32_t type02x2SegmentFlagsBack = 6;
        constexpr uint32_t type02x2SegmentFlagsFront = 7;
        constexpr uint32_t type02x2RoofNE = 8;
        constexpr uint32_t type02x2RoofSE = 9;
        constexpr uint32_t type02x2RoofSW = 10;
        constexpr uint32_t type02x2RoofNW = 11;
        constexpr uint32_t type11x1SegmentFlagsBack = 12;
        constexpr uint32_t type11x1SegmentFlagsFront = 13;
        constexpr uint32_t type11x1RoofNE = 14;
        constexpr uint32_t type11x1RoofSE = 15;
        constexpr uint32_t type11x1RoofSW = 16;
        constexpr uint32_t type11x1RoofNW = 17;
        constexpr uint32_t type12x2SegmentFlagsBack = 18;
        constexpr uint32_t type12x2SegmentFlagsFront = 19;
        constexpr uint32_t type12x2RoofNE = 20;
        constexpr uint32_t type12x2RoofSE = 21;
        constexpr uint32_t type12x2RoofSW = 22;
        constexpr uint32_t type12x2RoofNW = 23;
        constexpr uint32_t type21x1SegmentFlagsBack = 24;
        constexpr uint32_t type21x1SegmentFlagsFront = 25;
        constexpr uint32_t type21x1RoofNE = 26;
        constexpr uint32_t type21x1RoofSE = 27;
        constexpr uint32_t type21x1RoofSW = 28;
        constexpr uint32_t type21x1RoofNW = 29;
        constexpr uint32_t type22x2SegmentFlagsBack = 30;
        constexpr uint32_t type22x2SegmentFlagsFront = 31;
        constexpr uint32_t type22x2RoofNE = 32;
        constexpr uint32_t type22x2RoofSE = 33;
        constexpr uint32_t type22x2RoofSW = 34;
        constexpr uint32_t type22x2RoofNW = 35;
    }

    struct ScaffoldingImages
    {
        struct Building
        {
            uint32_t back;
            uint32_t front;
            std::array<uint32_t, 4> roof;
            constexpr uint32_t getRoof(const uint8_t rotation) const { return roof[rotation & 0x3]; }
        };

        Building buildings[2];
        constexpr const Building& get1x1() const { return buildings[0]; }
        constexpr const Building& get2x2() const { return buildings[1]; }
    };

    static constexpr std::array<ScaffoldingImages, 3> kScaffoldingImages = {
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type01x1SegmentFlagsBack,
                Scaffolding::ImageIds::type01x1SegmentFlagsFront,
                {
                    Scaffolding::ImageIds::type01x1RoofNE,
                    Scaffolding::ImageIds::type01x1RoofSE,
                    Scaffolding::ImageIds::type01x1RoofSW,
                    Scaffolding::ImageIds::type01x1RoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type02x2SegmentFlagsBack,
                Scaffolding::ImageIds::type02x2SegmentFlagsFront,
                {
                    Scaffolding::ImageIds::type02x2RoofNE,
                    Scaffolding::ImageIds::type02x2RoofSE,
                    Scaffolding::ImageIds::type02x2RoofSW,
                    Scaffolding::ImageIds::type02x2RoofNW,
                },
            },
        },
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type11x1SegmentFlagsBack,
                Scaffolding::ImageIds::type11x1SegmentFlagsFront,
                {
                    Scaffolding::ImageIds::type11x1RoofNE,
                    Scaffolding::ImageIds::type11x1RoofSE,
                    Scaffolding::ImageIds::type11x1RoofSW,
                    Scaffolding::ImageIds::type11x1RoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type12x2SegmentFlagsBack,
                Scaffolding::ImageIds::type12x2SegmentFlagsFront,
                {
                    Scaffolding::ImageIds::type12x2RoofNE,
                    Scaffolding::ImageIds::type12x2RoofSE,
                    Scaffolding::ImageIds::type12x2RoofSW,
                    Scaffolding::ImageIds::type12x2RoofNW,
                },
            },
        },
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type21x1SegmentFlagsBack,
                Scaffolding::ImageIds::type21x1SegmentFlagsFront,
                {
                    Scaffolding::ImageIds::type21x1RoofNE,
                    Scaffolding::ImageIds::type21x1RoofSE,
                    Scaffolding::ImageIds::type21x1RoofSW,
                    Scaffolding::ImageIds::type21x1RoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type22x2SegmentFlagsBack,
                Scaffolding::ImageIds::type22x2SegmentFlagsFront,
                {
                    Scaffolding::ImageIds::type22x2RoofNE,
                    Scaffolding::ImageIds::type22x2RoofSE,
                    Scaffolding::ImageIds::type22x2RoofSW,
                    Scaffolding::ImageIds::type22x2RoofNW,
                },
            },
        },
    };

    constexpr const ScaffoldingImages& getScaffoldingImages(uint8_t type) { return kScaffoldingImages[type]; }
}
