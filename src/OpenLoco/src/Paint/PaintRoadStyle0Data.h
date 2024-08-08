#include "Objects/RoadObject.h"
#include "Paint.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <array>
#include <span>

namespace OpenLoco::Paint::Style0
{
    struct RoadPaintPiece
    {
    private:
        constexpr void rotateTunnelHeights()
        {
            streetlightHeights[1][0] = streetlightHeights[0][3];
            streetlightHeights[1][1] = streetlightHeights[0][0];
            streetlightHeights[1][2] = streetlightHeights[0][1];
            streetlightHeights[1][3] = streetlightHeights[0][2];

            streetlightHeights[2][0] = streetlightHeights[0][2];
            streetlightHeights[2][1] = streetlightHeights[0][3];
            streetlightHeights[2][2] = streetlightHeights[0][0];
            streetlightHeights[2][3] = streetlightHeights[0][1];

            streetlightHeights[3][0] = streetlightHeights[0][1];
            streetlightHeights[3][1] = streetlightHeights[0][2];
            streetlightHeights[3][2] = streetlightHeights[0][3];
            streetlightHeights[3][3] = streetlightHeights[0][0];
        }

    public:
        constexpr RoadPaintPiece(
            const std::array<uint32_t, 4>& _imageIndexOffsets,
            const std::array<int16_t, 4>& _tunnelHeights,
            const bool _isMultiTileMerge)
            : imageIndexOffsets(_imageIndexOffsets)
            , streetlightHeights()
            , isMultiTileMerge(_isMultiTileMerge)
        {
            streetlightHeights = {};
            streetlightHeights[0] = _tunnelHeights;
            rotateTunnelHeights();
        }

        std::array<uint32_t, 4> imageIndexOffsets;
        std::array<std::array<int16_t, 4>, 4> streetlightHeights;
        bool isMultiTileMerge;
    };

    constexpr int16_t kNoStreetlight = -1;
    constexpr std::array<int16_t, 4> kNoStreetlights = { kNoStreetlight, kNoStreetlight, kNoStreetlight, kNoStreetlight };
    constexpr std::array<uint8_t, 4> kRotationTable1230 = { 1, 2, 3, 0 };
    constexpr std::array<uint8_t, 4> kRotationTable2301 = { 2, 3, 0, 1 };
    constexpr std::array<uint8_t, 4> kRotationTable3012 = { 3, 0, 1, 2 };

    consteval RoadPaintPiece rotateRoadPP(const RoadPaintPiece& reference, const std::array<uint8_t, 4>& rotationTable)
    {
        return RoadPaintPiece{
            std::array<uint32_t, 4>{
                reference.imageIndexOffsets[rotationTable[0]],
                reference.imageIndexOffsets[rotationTable[1]],
                reference.imageIndexOffsets[rotationTable[2]],
                reference.imageIndexOffsets[rotationTable[3]],
            },
            std::array<int16_t, 4>{
                reference.streetlightHeights[0][rotationTable[0]],
                reference.streetlightHeights[0][rotationTable[1]],
                reference.streetlightHeights[0][rotationTable[2]],
                reference.streetlightHeights[0][rotationTable[3]],
            },
            reference.isMultiTileMerge
        };
    }

    // 0x004083B1, 0x004084BC, 0x004083B1, 0x004084BC
    constexpr RoadPaintPiece kStraight0 = {
        std::array<uint32_t, 4>{ 3486, 3487, 3486, 3487 },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ true,
    };

    constexpr std::array<RoadPaintPiece, 1> kStraightTPP = {
        kStraight0,
    };

    // 0x004087DD, 0x004088E8, 0x004089F3, 0x00408AFC
    constexpr RoadPaintPiece kRightCurveVerySmall0 = {
        std::array<uint32_t, 4>{ 3488, 3489, 3490, 3491 },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ true,
    };

    constexpr std::array<RoadPaintPiece, 1> kRightCurveVerySmallTPP = {
        kRightCurveVerySmall0,
    };

    constexpr RoadPaintPiece kLeftCurveVerySmall0 = rotateRoadPP(kRightCurveVerySmall0, kRotationTable1230);

    constexpr std::array<RoadPaintPiece, 1> kLeftCurveVerySmallTPP = {
        kLeftCurveVerySmall0,
    };

    // 0x0040935D, 0x004096C9, 0x00409A37, 0x00409DA3
    constexpr RoadPaintPiece kRightCurveSmall0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kRightCurveSmall0NE,
            RoadObj::ImageIds::Style0::kRightCurveSmall0SE,
            RoadObj::ImageIds::Style0::kRightCurveSmall0SW,
            RoadObj::ImageIds::Style0::kRightCurveSmall0NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x0040948D, 0x004097F9, 0x00409B67, 0x00409ED3
    constexpr RoadPaintPiece kRightCurveSmall1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kRightCurveSmall1NE,
            RoadObj::ImageIds::Style0::kRightCurveSmall1SE,
            RoadObj::ImageIds::Style0::kRightCurveSmall1SW,
            RoadObj::ImageIds::Style0::kRightCurveSmall1NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x00409512, 0x00409880, 0x00409BEE, 0x00409F5A
    constexpr RoadPaintPiece kRightCurveSmall2 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kRightCurveSmall2NE,
            RoadObj::ImageIds::Style0::kRightCurveSmall2SE,
            RoadObj::ImageIds::Style0::kRightCurveSmall2SW,
            RoadObj::ImageIds::Style0::kRightCurveSmall2NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x00409599, 0x00409907, 0x00409C73, 0x00409FE1
    constexpr RoadPaintPiece kRightCurveSmall3 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kRightCurveSmall3NE,
            RoadObj::ImageIds::Style0::kRightCurveSmall3SE,
            RoadObj::ImageIds::Style0::kRightCurveSmall3SW,
            RoadObj::ImageIds::Style0::kRightCurveSmall3NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    constexpr std::array<RoadPaintPiece, 4> kRightCurveSmallTPP = {
        kRightCurveSmall0,
        kRightCurveSmall1,
        kRightCurveSmall2,
        kRightCurveSmall3,
    };

    constexpr RoadPaintPiece kLeftCurveSmall0 = rotateRoadPP(kRightCurveSmall3, kRotationTable1230);

    constexpr RoadPaintPiece kLeftCurveSmall1 = rotateRoadPP(kRightCurveSmall1, kRotationTable1230);

    constexpr RoadPaintPiece kLeftCurveSmall2 = rotateRoadPP(kRightCurveSmall2, kRotationTable1230);

    constexpr RoadPaintPiece kLeftCurveSmall3 = rotateRoadPP(kRightCurveSmall0, kRotationTable1230);

    constexpr std::array<RoadPaintPiece, 4> kLeftCurveSmallTPP = {
        kLeftCurveSmall0,
        kLeftCurveSmall1,
        kLeftCurveSmall2,
        kLeftCurveSmall3,
    };

    // 0x0040AF19, 0x0040B27F, 0x0040B5E5, 0x0040B94B
    constexpr RoadPaintPiece kStraightSlopeUp0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kStraightSlopeUp0NE,
            RoadObj::ImageIds::Style0::kStraightSlopeUp0SE,
            RoadObj::ImageIds::Style0::kStraightSlopeUp0SW,
            RoadObj::ImageIds::Style0::kStraightSlopeUp0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            4,
            kNoStreetlight,
            4,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ false,
    };

    // 0x0040B0CA, 0x0040B430, 0x0040B796, 0x0040BAFC
    constexpr RoadPaintPiece kStraightSlopeUp1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kStraightSlopeUp1NE,
            RoadObj::ImageIds::Style0::kStraightSlopeUp1SE,
            RoadObj::ImageIds::Style0::kStraightSlopeUp1SW,
            RoadObj::ImageIds::Style0::kStraightSlopeUp1NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            12,
            kNoStreetlight,
            12,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ false,
    };

    constexpr std::array<RoadPaintPiece, 2> kStraightSlopeUpTPP = {
        kStraightSlopeUp0,
        kStraightSlopeUp1,
    };

    constexpr RoadPaintPiece kStraightSlopeDown0 = rotateRoadPP(kStraightSlopeUp1, kRotationTable2301);

    constexpr RoadPaintPiece kStraightSlopeDown1 = rotateRoadPP(kStraightSlopeUp0, kRotationTable2301);

    constexpr std::array<RoadPaintPiece, 2> kStraightSlopeDownTPP = {
        kStraightSlopeDown0,
        kStraightSlopeDown1,
    };

    // 0x0040CA49, 0x0040CC2E, 0x0040CE13, 0x0040CFF8
    constexpr RoadPaintPiece kStraightSteepSlopeUp0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style0::kStraightSteepSlopeUp0NE,
            RoadObj::ImageIds::Style0::kStraightSteepSlopeUp0SE,
            RoadObj::ImageIds::Style0::kStraightSteepSlopeUp0SW,
            RoadObj::ImageIds::Style0::kStraightSteepSlopeUp0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            8,
            kNoStreetlight,
            8,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ false,
    };

    constexpr std::array<RoadPaintPiece, 1> kStraightSteepSlopeUpTPP = {
        kStraightSteepSlopeUp0,
    };

    constexpr RoadPaintPiece kStraightSteepSlopeDown0 = rotateRoadPP(kStraightSteepSlopeUp0, kRotationTable2301);

    constexpr std::array<RoadPaintPiece, 1> kStraightSteepSlopeDownTPP = {
        kStraightSteepSlopeDown0,
    };

    constexpr std::array<std::span<const RoadPaintPiece>, 9> kRoadPaintParts = {
        kStraightTPP,
        kLeftCurveVerySmallTPP,
        kRightCurveVerySmallTPP,
        kLeftCurveSmallTPP,
        kRightCurveSmallTPP,
        kStraightSlopeUpTPP,
        kStraightSlopeDownTPP,
        kStraightSteepSlopeUpTPP,
        kStraightSteepSlopeDownTPP,
    };
}
