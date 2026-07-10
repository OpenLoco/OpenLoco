#pragma once
#include "Graphics/ImageIds.h"
#include "Objects/RoadObject.h"
#include "Paint.h"
#include "PaintRoadCommonData.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <array>
#include <span>

namespace OpenLoco::Paint::Style2
{
    // 0x004083B1, 0x004084BC, 0x004085C7, 0x004086D2
    constexpr RoadPaintMergeablePiece kStraight0 = {
        std::array<uint32_t, 4>{
            ImageIds::road_hit_test_straight_NE,
            ImageIds::road_hit_test_straight_SW,
            ImageIds::road_hit_test_straight_NE,
            ImageIds::road_hit_test_straight_SW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ std::array<RoadPaintMergeType, 4>{ RoadPaintMergeType::left, RoadPaintMergeType::left, RoadPaintMergeType::right, RoadPaintMergeType::right },
    };

    constexpr std::array<RoadPaintMergeablePiece, 1> kStraightTPP = {
        kStraight0,
    };

    // 0x00408C07, 0x00408D12, 0x00408E1D, 0x00408F26
    constexpr RoadPaintMergeablePiece kRightCurveVerySmall0 = {
        std::array<uint32_t, 4>{
            ImageIds::road_hit_test_very_small_curve_right_NE,
            ImageIds::road_hit_test_very_small_curve_right_SE,
            ImageIds::road_hit_test_very_small_curve_right_SW,
            ImageIds::road_hit_test_very_small_curve_right_NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ std::array<RoadPaintMergeType, 4>{ RoadPaintMergeType::right, RoadPaintMergeType::right, RoadPaintMergeType::right, RoadPaintMergeType::right },
    };

    constexpr std::array<RoadPaintMergeablePiece, 1> kRightCurveVerySmallTPP = {
        kRightCurveVerySmall0,
    };

    // 0x004088E8, 0x004089F3, 0x00408AFC, 0x004087DD
    constexpr RoadPaintMergeablePiece kLeftCurveVerySmall0 = {
        std::array<uint32_t, 4>{
            ImageIds::road_hit_test_very_small_curve_right_SE,
            ImageIds::road_hit_test_very_small_curve_right_SW,
            ImageIds::road_hit_test_very_small_curve_right_NW,
            ImageIds::road_hit_test_very_small_curve_right_NE,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ std::array<RoadPaintMergeType, 4>{ RoadPaintMergeType::left, RoadPaintMergeType::left, RoadPaintMergeType::left, RoadPaintMergeType::left },
    };

    constexpr std::array<RoadPaintMergeablePiece, 1> kLeftCurveVerySmallTPP = {
        kLeftCurveVerySmall0,
    };

    // 0x0040A111, 0x0040A47D, 0x0040A7EB, 0x0040AB57
    constexpr RoadPaintMergeablePiece kRightCurveSmall0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall0NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall0SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall0SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            kNoStreetlight,
            kNoStreetlight,
            0,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x0040A241, 0x0040A5AD, 0x0040A91B, 0x0040AC87
    constexpr RoadPaintMergeablePiece kRightCurveSmall1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall1NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall1SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall1SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall1NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x0040A2C6, 0x0040A634, 0x0040A9A2, 0x0040AD0E
    constexpr RoadPaintMergeablePiece kRightCurveSmall2 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall2NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall2SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall2SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall2NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x0040A34D, 0x0040A6BB, 0x0040AA27, 0x0040AD95
    constexpr RoadPaintMergeablePiece kRightCurveSmall3 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kRightCurveSmall3NE,
            RoadObj::ImageIds::Style2::kRightCurveSmall3SE,
            RoadObj::ImageIds::Style2::kRightCurveSmall3SW,
            RoadObj::ImageIds::Style2::kRightCurveSmall3NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            0,
            kNoStreetlight,
            kNoStreetlight,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 4> kRightCurveSmallTPP = {
        kRightCurveSmall0,
        kRightCurveSmall1,
        kRightCurveSmall2,
        kRightCurveSmall3,
    };

    // 0x00409907, 0x00409C73, 0x00409FE1, 0x00409599
    constexpr RoadPaintMergeablePiece kLeftCurveSmall0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall0NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall0SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall0SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            0,
            kNoStreetlight,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x004097F9, 0x00409B67, 0x00409ED3, 0x0040948D
    constexpr RoadPaintMergeablePiece kLeftCurveSmall1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall1NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall1SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall1SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall1NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x00409880, 0x00409BEE, 0x00409F5A, 0x00409512
    constexpr RoadPaintMergeablePiece kLeftCurveSmall2 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall2NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall2SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall2SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall2NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x004096C9, 0x00409A37, 0x00409DA3, 0x0040935D
    constexpr RoadPaintMergeablePiece kLeftCurveSmall3 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kLeftCurveSmall3NE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall3SE,
            RoadObj::ImageIds::Style2::kLeftCurveSmall3SW,
            RoadObj::ImageIds::Style2::kLeftCurveSmall3NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            0,
            kNoStreetlight,
            kNoStreetlight,
            kNoStreetlight,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 4> kLeftCurveSmallTPP = {
        kLeftCurveSmall0,
        kLeftCurveSmall1,
        kLeftCurveSmall2,
        kLeftCurveSmall3,
    };

    // 0x0040AF19, 0x0040B27F, 0x0040B5E5, 0x0040B94B
    constexpr RoadPaintMergeablePiece kStraightSlopeUp0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeUp0NE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp0SE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp0SW,
            RoadObj::ImageIds::Style2::kStraightSlopeUp0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            4,
            kNoStreetlight,
            4,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x0040B0CA, 0x0040B430, 0x0040B796, 0x0040BAFC
    constexpr RoadPaintMergeablePiece kStraightSlopeUp1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeUp1NE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp1SE,
            RoadObj::ImageIds::Style2::kStraightSlopeUp1SW,
            RoadObj::ImageIds::Style2::kStraightSlopeUp1NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            12,
            kNoStreetlight,
            12,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 2> kStraightSlopeUpTPP = {
        kStraightSlopeUp0,
        kStraightSlopeUp1,
    };

    // 0x0040C52E, 0x0040C894, 0x0040BE62, 0x0040C1C8
    constexpr RoadPaintMergeablePiece kStraightSlopeDown0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeDown0NE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown0SE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown0SW,
            RoadObj::ImageIds::Style2::kStraightSlopeDown0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            12,
            kNoStreetlight,
            12,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    // 0x0040C37D, 0x0040C6E3, 0x0040BCB1, 0x0040C017
    constexpr RoadPaintMergeablePiece kStraightSlopeDown1 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSlopeDown1NE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown1SE,
            RoadObj::ImageIds::Style2::kStraightSlopeDown1SW,
            RoadObj::ImageIds::Style2::kStraightSlopeDown1NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            4,
            kNoStreetlight,
            4,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 2> kStraightSlopeDownTPP = {
        kStraightSlopeDown0,
        kStraightSlopeDown1,
    };

    // 0x0040CA49, 0x0040CC2E, 0x0040CE13, 0x0040CFF8
    constexpr RoadPaintMergeablePiece kStraightSteepSlopeUp0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0NE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0SE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0SW,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeUp0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            8,
            kNoStreetlight,
            8,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 1> kStraightSteepSlopeUpTPP = {
        kStraightSteepSlopeUp0,
    };

    // 0x0040D5A7, 0x0040D78C, 0x0040D1DD, 0x0040D3C2
    constexpr RoadPaintMergeablePiece kStraightSteepSlopeDown0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0NE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0SE,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0SW,
            RoadObj::ImageIds::Style2::kStraightSteepSlopeDown0NW,
        },
        /* StreetlightHeights */ std::array<int16_t, 4>{
            kNoStreetlight,
            8,
            kNoStreetlight,
            8,
        },
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 1> kStraightSteepSlopeDownTPP = {
        kStraightSteepSlopeDown0,
    };

    // 0x00409031, 0x004090E8, 0x0040919D, 0x00409252
    constexpr RoadPaintMergeablePiece kTurnaround0 = {
        std::array<uint32_t, 4>{
            RoadObj::ImageIds::Style2::kTurnaround0NE,
            RoadObj::ImageIds::Style2::kTurnaround0SE,
            RoadObj::ImageIds::Style2::kTurnaround0SW,
            RoadObj::ImageIds::Style2::kTurnaround0NW,
        },
        /* StreetlightHeights */ kNoStreetlights,
        /* IsMultiTileMerge */ kNoRoadPaintMerge,
    };

    constexpr std::array<RoadPaintMergeablePiece, 1> kTurnaroundTPP = {
        kTurnaround0,
    };

    constexpr std::array<std::span<const RoadPaintMergeablePiece>, 10> kRoadPaintParts = {
        kStraightTPP,
        kLeftCurveVerySmallTPP,
        kRightCurveVerySmallTPP,
        kLeftCurveSmallTPP,
        kRightCurveSmallTPP,
        kStraightSlopeUpTPP,
        kStraightSlopeDownTPP,
        kStraightSteepSlopeUpTPP,
        kStraightSteepSlopeDownTPP,
        kTurnaroundTPP,
    };

}
