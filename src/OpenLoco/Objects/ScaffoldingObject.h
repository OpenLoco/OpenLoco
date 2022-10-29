#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
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
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(ScaffoldingObject) == 0x12);

    namespace Scaffolding::ImageIds
    {
        constexpr uint32_t type0industrySegmentPart0 = 0;
        constexpr uint32_t type0industrySegmentPart1 = 1;
        constexpr uint32_t type0industrySegmentRoofNE = 2;
        constexpr uint32_t type0industrySegmentRoofSE = 3;
        constexpr uint32_t type0industrySegmentRoofSW = 4;
        constexpr uint32_t type0industrySegmentRoofNW = 5;
        constexpr uint32_t type0buildingSegmentPart0 = 6;
        constexpr uint32_t type0buildingSegmentPart1 = 7;
        constexpr uint32_t type0buildingSegmentRoofNE = 8;
        constexpr uint32_t type0buildingSegmentRoofSE = 9;
        constexpr uint32_t type0buildingSegmentRoofSW = 10;
        constexpr uint32_t type0buildingSegmentRoofNW = 11;
        constexpr uint32_t type1industrySegmentPart0 = 12;
        constexpr uint32_t type1industrySegmentPart1 = 13;
        constexpr uint32_t type1industrySegmentRoofNE = 14;
        constexpr uint32_t type1industrySegmentRoofSE = 15;
        constexpr uint32_t type1industrySegmentRoofSW = 16;
        constexpr uint32_t type1industrySegmentRoofNW = 17;
        constexpr uint32_t type1buildingSegmentPart0 = 18;
        constexpr uint32_t type1buildingSegmentPart1 = 19;
        constexpr uint32_t type1buildingSegmentRoofNE = 20;
        constexpr uint32_t type1buildingSegmentRoofSE = 21;
        constexpr uint32_t type1buildingSegmentRoofSW = 22;
        constexpr uint32_t type1buildingSegmentRoofNW = 23;
        constexpr uint32_t type2industrySegmentPart0 = 24;
        constexpr uint32_t type2industrySegmentPart1 = 25;
        constexpr uint32_t type2industrySegmentRoofNE = 26;
        constexpr uint32_t type2industrySegmentRoofSE = 27;
        constexpr uint32_t type2industrySegmentRoofSW = 28;
        constexpr uint32_t type2industrySegmentRoofNW = 29;
        constexpr uint32_t type2buildingSegmentPart0 = 30;
        constexpr uint32_t type2buildingSegmentPart1 = 31;
        constexpr uint32_t type2buildingSegmentRoofNE = 32;
        constexpr uint32_t type2buildingSegmentRoofSE = 33;
        constexpr uint32_t type2buildingSegmentRoofSW = 34;
        constexpr uint32_t type2buildingSegmentRoofNW = 35;
    }

    struct ScaffoldingImages
    {
        struct Building
        {
            uint32_t part0;
            uint32_t part1;
            std::array<uint32_t, 4> roof;
            constexpr uint32_t getRoof(const uint8_t rotation) const { return roof[rotation & 0x3]; }
        };

        Building buildings[2];
        constexpr const Building& getIndustry() const { return buildings[0]; }
        constexpr const Building& getBuilding() const { return buildings[1]; }
    };

    static constexpr std::array<ScaffoldingImages, 3> kScaffoldingImages = {
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type0industrySegmentPart0,
                Scaffolding::ImageIds::type0industrySegmentPart1,
                {
                    Scaffolding::ImageIds::type0industrySegmentRoofNE,
                    Scaffolding::ImageIds::type0industrySegmentRoofSE,
                    Scaffolding::ImageIds::type0industrySegmentRoofSW,
                    Scaffolding::ImageIds::type0industrySegmentRoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type0buildingSegmentPart0,
                Scaffolding::ImageIds::type0buildingSegmentPart1,
                {
                    Scaffolding::ImageIds::type0buildingSegmentRoofNE,
                    Scaffolding::ImageIds::type0buildingSegmentRoofSE,
                    Scaffolding::ImageIds::type0buildingSegmentRoofSW,
                    Scaffolding::ImageIds::type0buildingSegmentRoofNW,
                },
            },
        },
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type1industrySegmentPart0,
                Scaffolding::ImageIds::type1industrySegmentPart1,
                {
                    Scaffolding::ImageIds::type1industrySegmentRoofNE,
                    Scaffolding::ImageIds::type1industrySegmentRoofSE,
                    Scaffolding::ImageIds::type1industrySegmentRoofSW,
                    Scaffolding::ImageIds::type1industrySegmentRoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type1buildingSegmentPart0,
                Scaffolding::ImageIds::type1buildingSegmentPart1,
                {
                    Scaffolding::ImageIds::type1buildingSegmentRoofNE,
                    Scaffolding::ImageIds::type1buildingSegmentRoofSE,
                    Scaffolding::ImageIds::type1buildingSegmentRoofSW,
                    Scaffolding::ImageIds::type1buildingSegmentRoofNW,
                },
            },
        },
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type2industrySegmentPart0,
                Scaffolding::ImageIds::type2industrySegmentPart1,
                {
                    Scaffolding::ImageIds::type2industrySegmentRoofNE,
                    Scaffolding::ImageIds::type2industrySegmentRoofSE,
                    Scaffolding::ImageIds::type2industrySegmentRoofSW,
                    Scaffolding::ImageIds::type2industrySegmentRoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type2buildingSegmentPart0,
                Scaffolding::ImageIds::type2buildingSegmentPart1,
                {
                    Scaffolding::ImageIds::type2buildingSegmentRoofNE,
                    Scaffolding::ImageIds::type2buildingSegmentRoofSE,
                    Scaffolding::ImageIds::type2buildingSegmentRoofSW,
                    Scaffolding::ImageIds::type2buildingSegmentRoofNW,
                },
            },
        },
    };

    constexpr const ScaffoldingImages& getScaffoldingImages(uint8_t type) { return kScaffoldingImages[type]; }
}
