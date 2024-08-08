#include "Objects/RoadObject.h"
#include "Paint.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <array>
#include <span>

namespace OpenLoco::Paint::Style1
{
    struct RoadPaintPiece
    {
    private:
        constexpr void rotateTunnelHeights()
        {
            tunnelHeights[1][0] = tunnelHeights[0][3];
            tunnelHeights[1][1] = tunnelHeights[0][0];
            tunnelHeights[1][2] = tunnelHeights[0][1];
            tunnelHeights[1][3] = tunnelHeights[0][2];

            tunnelHeights[2][0] = tunnelHeights[0][2];
            tunnelHeights[2][1] = tunnelHeights[0][3];
            tunnelHeights[2][2] = tunnelHeights[0][0];
            tunnelHeights[2][3] = tunnelHeights[0][1];

            tunnelHeights[3][0] = tunnelHeights[0][1];
            tunnelHeights[3][1] = tunnelHeights[0][2];
            tunnelHeights[3][2] = tunnelHeights[0][3];
            tunnelHeights[3][3] = tunnelHeights[0][0];
        }

    public:
        constexpr RoadPaintPiece(
            const std::array<uint32_t, 4>& _imageIndexOffsets,
            const std::array<int16_t, 4>& _tunnelHeights,
            const bool _isMultiTileMerge)
            : imageIndexOffsets(_imageIndexOffsets)
            , tunnelHeights()
            , isMultiTileMerge(_isMultiTileMerge)
        {
            tunnelHeights = {};
            tunnelHeights[0] = _tunnelHeights;
            rotateTunnelHeights();
        }

        std::array<uint32_t, 4> imageIndexOffsets;
        std::array<std::array<int16_t, 4>, 4> tunnelHeights;
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
                reference.tunnelHeights[0][rotationTable[0]],
                reference.tunnelHeights[0][rotationTable[1]],
                reference.tunnelHeights[0][rotationTable[2]],
                reference.tunnelHeights[0][rotationTable[3]],
            },
            reference.isMultiTileMerge
        };
    }
    // 0x004083B1, 0x004084BC, 0x004085C7, 0x004086D2
    constexpr RoadPaintPiece kStraight0 = {
        std::array<uint32_t, 4>{ 3486, 3487, 3486, 3487 },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ true,
    };

    constexpr std::array<RoadPaintPiece, 1> kStraightTPP = {
        kStraight0,
    };

    // 0x00408C07, 0x00408D12, 0x00408E1D, 0x00408F26
    constexpr RoadPaintPiece kRightCurveVerySmall0 = {
        std::array<uint32_t, 4>{ 3488, 3489, 3490, 3491 },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ true,
    };

    constexpr std::array<RoadPaintPiece, 1> kRightCurveVerySmallTPP = {
        kRightCurveVerySmall0,
    };

    // 0x004088E8, 0x004089F3, 0x00408AFC, 0x004087DD
    constexpr RoadPaintPiece kLeftCurveVerySmall0 = {
        std::array<uint32_t, 4>{ 3489, 3490, 3491, 3488 },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ true,
    };

    constexpr std::array<RoadPaintPiece, 1> kLeftCurveVerySmallTPP = {
        kLeftCurveVerySmall0,
    };

    // 0x0040A111, 0x0040A47D, 0x0040A7EB, 0x0040AB57
    constexpr RoadPaintPiece kRightCurveSmall0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall0NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall0SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall0SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall0NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x0040A241, 0x0040A5AD, 0x0040A91B, 0x0040AC87
    constexpr RoadPaintPiece kRightCurveSmall1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall1NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall1SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall1SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall1NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x0040A2C6, 0x0040A634, 0x0040A9A2, 0x0040AD0E
    constexpr RoadPaintPiece kRightCurveSmall2 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall2NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall2SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall2SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall2NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x0040A34D, 0x0040A6BB, 0x0040AA27, 0x0040AD95
    constexpr RoadPaintPiece kRightCurveSmall3 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall3NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall3SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall3SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall3NW,
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

    // 0x00409907, 0x00409C73, 0x00409FE1, 0x00409599
    constexpr RoadPaintPiece kLeftCurveSmall0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall0NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall0SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall0SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall0NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x004097F9, 0x00409B67, 0x00409ED3, 0x0040948D
    constexpr RoadPaintPiece kLeftCurveSmall1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall1NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall1SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall1SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall1NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x00409880, 0x00409BEE, 0x00409F5A, 0x00409512
    constexpr RoadPaintPiece kLeftCurveSmall2 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall2NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall2SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall2SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall2NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    // 0x004096C9, 0x00409A37, 0x00409DA3, 0x0040935D
    constexpr RoadPaintPiece kLeftCurveSmall3 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall3NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall3SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall3SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall3NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ false,
    };

    constexpr std::array<RoadPaintPiece, 4> kLeftCurveSmallTPP = {
        kLeftCurveSmall0,
        kLeftCurveSmall1,
        kLeftCurveSmall2,
        kLeftCurveSmall3,
    };

    // 0x0040AF19, 0x0040B27F, 0x0040B5E5, 0x0040B94B
    constexpr RoadPaintPiece kStraightSlopeUp0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeUp0NE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp0SE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp0SW,
            RoadObj::ImageIds::Style2::kStraightSlopeUp0NW,
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
            RoadObj::ImageIds::Style2::kStraightSlopeUp1NE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp1SE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp1SW,
            RoadObj::ImageIds::Style2::kStraightSlopeUp1NW,
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

    // 0x0040C52E, 0x0040C894, 0x0040BE62, 0x0040C1C8
    constexpr RoadPaintPiece kStraightSlopeDown0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeDown0NE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown0SE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown0SW,
            RoadObj::ImageIds::Style2::kStraightSlopeDown0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            12,
            kNoStreetlight,
            12,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ false,
    };

    // 0x0040C37D, 0x0040C6E3, 0x0040BCB1, 0x0040C017
    constexpr RoadPaintPiece kStraightSlopeDown1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeDown1NE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown1SE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown1SW,
            RoadObj::ImageIds::Style2::kStraightSlopeDown1NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            4,
            kNoStreetlight,
            4,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ false,
    };

    constexpr std::array<RoadPaintPiece, 2> kStraightSlopeDownTPP = {
        kStraightSlopeDown0,
        kStraightSlopeDown1,
    };

    // 0x0040CA49, 0x0040CC2E, 0x0040CE13, 0x0040CFF8
    constexpr RoadPaintPiece kStraightSteepSlopeUp0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0NE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0SE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0SW,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0NW,
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

    // 0x0040D5A7, 0x0040D78C, 0x0040D1DD, 0x0040D3C2
    constexpr RoadPaintPiece kStraightSteepSlopeDown0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0NE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0SE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0SW,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            8,
            kNoStreetlight,
            8,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ false,
    };

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
