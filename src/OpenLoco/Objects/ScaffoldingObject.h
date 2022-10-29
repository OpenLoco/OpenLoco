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
        constexpr uint32_t type01x1SegmentPart0 = 0;
        constexpr uint32_t type01x1SegmentPart1 = 1;
        constexpr uint32_t type01x1SegmentRoofNE = 2;
        constexpr uint32_t type01x1SegmentRoofSE = 3;
        constexpr uint32_t type01x1SegmentRoofSW = 4;
        constexpr uint32_t type01x1SegmentRoofNW = 5;
        constexpr uint32_t type02x2SegmentPart0 = 6;
        constexpr uint32_t type02x2SegmentPart1 = 7;
        constexpr uint32_t type02x2SegmentRoofNE = 8;
        constexpr uint32_t type02x2SegmentRoofSE = 9;
        constexpr uint32_t type02x2SegmentRoofSW = 10;
        constexpr uint32_t type02x2SegmentRoofNW = 11;
        constexpr uint32_t type11x1SegmentPart0 = 12;
        constexpr uint32_t type11x1SegmentPart1 = 13;
        constexpr uint32_t type11x1SegmentRoofNE = 14;
        constexpr uint32_t type11x1SegmentRoofSE = 15;
        constexpr uint32_t type11x1SegmentRoofSW = 16;
        constexpr uint32_t type11x1SegmentRoofNW = 17;
        constexpr uint32_t type12x2SegmentPart0 = 18;
        constexpr uint32_t type12x2SegmentPart1 = 19;
        constexpr uint32_t type12x2SegmentRoofNE = 20;
        constexpr uint32_t type12x2SegmentRoofSE = 21;
        constexpr uint32_t type12x2SegmentRoofSW = 22;
        constexpr uint32_t type12x2SegmentRoofNW = 23;
        constexpr uint32_t type21x1SegmentPart0 = 24;
        constexpr uint32_t type21x1SegmentPart1 = 25;
        constexpr uint32_t type21x1SegmentRoofNE = 26;
        constexpr uint32_t type21x1SegmentRoofSE = 27;
        constexpr uint32_t type21x1SegmentRoofSW = 28;
        constexpr uint32_t type21x1SegmentRoofNW = 29;
        constexpr uint32_t type22x2SegmentPart0 = 30;
        constexpr uint32_t type22x2SegmentPart1 = 31;
        constexpr uint32_t type22x2SegmentRoofNE = 32;
        constexpr uint32_t type22x2SegmentRoofSE = 33;
        constexpr uint32_t type22x2SegmentRoofSW = 34;
        constexpr uint32_t type22x2SegmentRoofNW = 35;
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
        constexpr const Building& get1x1() const { return buildings[0]; }
        constexpr const Building& get2x2() const { return buildings[1]; }
    };

    static constexpr std::array<ScaffoldingImages, 3> kScaffoldingImages = {
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type01x1SegmentPart0,
                Scaffolding::ImageIds::type01x1SegmentPart1,
                {
                    Scaffolding::ImageIds::type01x1SegmentRoofNE,
                    Scaffolding::ImageIds::type01x1SegmentRoofSE,
                    Scaffolding::ImageIds::type01x1SegmentRoofSW,
                    Scaffolding::ImageIds::type01x1SegmentRoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type02x2SegmentPart0,
                Scaffolding::ImageIds::type02x2SegmentPart1,
                {
                    Scaffolding::ImageIds::type02x2SegmentRoofNE,
                    Scaffolding::ImageIds::type02x2SegmentRoofSE,
                    Scaffolding::ImageIds::type02x2SegmentRoofSW,
                    Scaffolding::ImageIds::type02x2SegmentRoofNW,
                },
            },
        },
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type11x1SegmentPart0,
                Scaffolding::ImageIds::type11x1SegmentPart1,
                {
                    Scaffolding::ImageIds::type11x1SegmentRoofNE,
                    Scaffolding::ImageIds::type11x1SegmentRoofSE,
                    Scaffolding::ImageIds::type11x1SegmentRoofSW,
                    Scaffolding::ImageIds::type11x1SegmentRoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type12x2SegmentPart0,
                Scaffolding::ImageIds::type12x2SegmentPart1,
                {
                    Scaffolding::ImageIds::type12x2SegmentRoofNE,
                    Scaffolding::ImageIds::type12x2SegmentRoofSE,
                    Scaffolding::ImageIds::type12x2SegmentRoofSW,
                    Scaffolding::ImageIds::type12x2SegmentRoofNW,
                },
            },
        },
        ScaffoldingImages{
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type21x1SegmentPart0,
                Scaffolding::ImageIds::type21x1SegmentPart1,
                {
                    Scaffolding::ImageIds::type21x1SegmentRoofNE,
                    Scaffolding::ImageIds::type21x1SegmentRoofSE,
                    Scaffolding::ImageIds::type21x1SegmentRoofSW,
                    Scaffolding::ImageIds::type21x1SegmentRoofNW,
                },
            },
            ScaffoldingImages::Building{
                Scaffolding::ImageIds::type22x2SegmentPart0,
                Scaffolding::ImageIds::type22x2SegmentPart1,
                {
                    Scaffolding::ImageIds::type22x2SegmentRoofNE,
                    Scaffolding::ImageIds::type22x2SegmentRoofSE,
                    Scaffolding::ImageIds::type22x2SegmentRoofSW,
                    Scaffolding::ImageIds::type22x2SegmentRoofNW,
                },
            },
        },
    };

    constexpr const ScaffoldingImages& getScaffoldingImages(uint8_t type) { return kScaffoldingImages[type]; }
}
