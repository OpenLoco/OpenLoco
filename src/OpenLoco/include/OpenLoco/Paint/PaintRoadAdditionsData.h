#pragma once
#include "Objects/RoadExtraObject.h"
#include "Paint.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <array>
#include <optional>
#include <span>

namespace OpenLoco::Paint::AdditionStyle1
{

    struct RoadAdditionSupport
    {
        std::array<std::array<uint32_t, 4>, 4> imageIds;
        int16_t height;
        std::array<uint8_t, 4> frequencies;   // Make array
        std::array<SegmentFlags, 4> segments; // Make array

        constexpr RoadAdditionSupport(
            const std::array<std::array<uint32_t, 4>, 4>& _imageIds,
            const int16_t _height,
            const std::array<uint8_t, 4>& _frequencies,
            const std::array<SegmentFlags, 4>& _segments)
            : imageIds(_imageIds)
            , height(_height)
            , frequencies(_frequencies)
            , segments(_segments)
        {
        }

        constexpr RoadAdditionSupport(
            const std::array<std::array<uint32_t, 4>, 4>& _imageIds,
            const int16_t _height,
            const uint8_t _frequency,
            const SegmentFlags _segment)
            : imageIds(_imageIds)
            , height(_height)
            , frequencies()
            , segments()
        {
            frequencies[0] = _frequency;
            frequencies[1] = Numerics::rotl4bit(frequencies[0], 2);
            frequencies[2] = frequencies[0];
            frequencies[3] = frequencies[1];

            segments[0] = _segment;
            for (auto i = 1U; i < 4; ++i)
            {
                segments[i] = rotlSegmentFlags(segments[0], i);
            }
        }
    };

    struct RoadPaintAdditionPiece
    {
        std::array<uint32_t, 4> imageIds;
        std::array<World::Pos3, 4> boundingBoxOffsets;
        std::array<World::Pos3, 4> boundingBoxSizes;
        bool isIsMergeable;
        std::optional<RoadAdditionSupport> supports;
    };
    constexpr std::array<uint8_t, 4> kRotationTable1230 = { 1, 2, 3, 0 };
    constexpr std::array<uint8_t, 4> kRotationTable2301 = { 2, 3, 0, 1 };
    constexpr std::array<uint8_t, 4> kRotationTable3012 = { 3, 0, 1, 2 };

    constexpr RoadPaintAdditionPiece kNullRoadPaintAdditionPiece = {};
    constexpr auto kNoSupports = std::nullopt;

    consteval std::optional<RoadAdditionSupport> rotateRoadPPASupport(const std::optional<RoadAdditionSupport>& reference, const std::array<uint8_t, 4>& rotationTable)
    {
        if (!reference.has_value())
        {
            return std::nullopt;
        }
        return RoadAdditionSupport{
            std::array<std::array<uint32_t, 4>, 4>{
                reference->imageIds[rotationTable[0]],
                reference->imageIds[rotationTable[1]],
                reference->imageIds[rotationTable[2]],
                reference->imageIds[rotationTable[3]],
            },
            reference->height,
            std::array<uint8_t, 4>{
                reference->frequencies[rotationTable[0]],
                reference->frequencies[rotationTable[1]],
                reference->frequencies[rotationTable[2]],
                reference->frequencies[rotationTable[3]],
            },
            std::array<SegmentFlags, 4>{
                reference->segments[rotationTable[0]],
                reference->segments[rotationTable[1]],
                reference->segments[rotationTable[2]],
                reference->segments[rotationTable[3]],
            }
        };
    }

    consteval RoadPaintAdditionPiece rotateRoadPPA(const RoadPaintAdditionPiece& reference, const std::array<uint8_t, 4>& rotationTable)
    {
        return RoadPaintAdditionPiece{
            std::array<uint32_t, 4>{
                reference.imageIds[rotationTable[0]],
                reference.imageIds[rotationTable[1]],
                reference.imageIds[rotationTable[2]],
                reference.imageIds[rotationTable[3]],
            },
            std::array<World::Pos3, 4>{
                reference.boundingBoxOffsets[rotationTable[0]],
                reference.boundingBoxOffsets[rotationTable[1]],
                reference.boundingBoxOffsets[rotationTable[2]],
                reference.boundingBoxOffsets[rotationTable[3]],
            },
            std::array<World::Pos3, 4>{
                reference.boundingBoxSizes[rotationTable[0]],
                reference.boundingBoxSizes[rotationTable[1]],
                reference.boundingBoxSizes[rotationTable[2]],
                reference.boundingBoxSizes[rotationTable[3]],
            },
            reference.isIsMergeable,
            rotateRoadPPASupport(reference.supports, rotationTable)
        };
    }

    using namespace OpenLoco::RoadExtraObj::ImageIds::Style1;

    // 0x00410159, 0x0041020A, 0x00410159, 0x0041020A
    constexpr RoadPaintAdditionPiece kStraightAddition0 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kStraight0NE,
            kStraight0SE,
            kStraight0NE,
            kStraight0SE,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 28, 1 },
        },
        /* Mergable */ true,
        /* Supports */ RoadAdditionSupport{
            /* ImageIds */ std::array<std::array<uint32_t, 4>, 4>{
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
            },
            /* SupportHeight */ 0,
            /* Frequency */ 2,
            /* Segment */ SegmentFlags::x1y0 | SegmentFlags::x1y2,
        },
    };

    constexpr std::array<RoadPaintAdditionPiece, 1> kStraightTPPA = {
        kStraightAddition0,
    };

    // 0x004102BB, 0x00410302, 0x00410349, 0x00410390
    constexpr RoadPaintAdditionPiece kRightCurveVerySmallAddition0 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kRightCurveVerySmall0NE,
            kRightCurveVerySmall0SE,
            kRightCurveVerySmall0SW,
            kRightCurveVerySmall0NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 28, 1 },
        },
        /* Mergable */ true,
        /* Supports */ kNoSupports,
    };

    constexpr std::array<RoadPaintAdditionPiece, 1> kRightCurveVerySmallTPPA = {
        kRightCurveVerySmallAddition0,
    };

    constexpr RoadPaintAdditionPiece kLeftCurveVerySmallAddition0 = rotateRoadPPA(kRightCurveVerySmallAddition0, kRotationTable1230);

    constexpr std::array<RoadPaintAdditionPiece, 1> kLeftCurveVerySmallTPPA = {
        kLeftCurveVerySmallAddition0,
    };

    // 0x0041052B, 0x0041068F, 0x004107F3, 0x00410957
    constexpr RoadPaintAdditionPiece kRightCurveSmallAddition0 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kRightCurveSmall0NE,
            kRightCurveSmall0SE,
            kRightCurveSmall0SW,
            kRightCurveSmall0NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 6, 26 },
            World::Pos3{ 6, 2, 26 },
            World::Pos3{ 2, 6, 26 },
            World::Pos3{ 6, 2, 26 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
        },
        /* Mergable */ true,
        /* Supports */ RoadAdditionSupport{
            /* ImageIds */ std::array<std::array<uint32_t, 4>, 4>{
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
            },
            /* SupportHeight */ 0,
            /* Frequency */ 0,
            /* Segment */ SegmentFlags::x1y0 | SegmentFlags::x1y2,
        },
    };

    constexpr RoadPaintAdditionPiece kRightCurveSmallAddition1 = kNullRoadPaintAdditionPiece;

    constexpr RoadPaintAdditionPiece kRightCurveSmallAddition2 = kNullRoadPaintAdditionPiece;

    // 0x004105DE, 0x00410742, 0x004108A6, 0x00410A0A
    constexpr RoadPaintAdditionPiece kRightCurveSmallAddition3 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kRightCurveSmall3NE,
            kRightCurveSmall3SE,
            kRightCurveSmall3SW,
            kRightCurveSmall3NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 6, 2, 26 },
            World::Pos3{ 2, 6, 26 },
            World::Pos3{ 6, 2, 26 },
            World::Pos3{ 2, 6, 26 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 20, 1 },
        },
        /* Mergable */ true,
        /* Supports */ RoadAdditionSupport{
            /* ImageIds */ std::array<std::array<uint32_t, 4>, 4>{
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
            },
            /* SupportHeight */ 0,
            /* Frequency */ 0,
            /* Segment */ SegmentFlags::x0y1 | SegmentFlags::x2y1,
        },
    };

    constexpr std::array<RoadPaintAdditionPiece, 4> kRightCurveSmallTPPA = {
        kRightCurveSmallAddition0,
        kRightCurveSmallAddition1,
        kRightCurveSmallAddition2,
        kRightCurveSmallAddition3,
    };

    constexpr RoadPaintAdditionPiece kLeftCurveSmallAddition0 = rotateRoadPPA(kRightCurveSmallAddition3, kRotationTable1230);

    constexpr RoadPaintAdditionPiece kLeftCurveSmallAddition1 = kNullRoadPaintAdditionPiece;

    constexpr RoadPaintAdditionPiece kLeftCurveSmallAddition2 = kNullRoadPaintAdditionPiece;

    constexpr RoadPaintAdditionPiece kLeftCurveSmallAddition3 = rotateRoadPPA(kRightCurveSmallAddition0, kRotationTable1230);

    constexpr std::array<RoadPaintAdditionPiece, 4> kLeftCurveSmallTPPA = {
        kLeftCurveSmallAddition0,
        kLeftCurveSmallAddition1,
        kLeftCurveSmallAddition2,
        kLeftCurveSmallAddition3,
    };

    // 0x00410AF3, 0x00410C83, 0x00410E13, 0x00410FA3
    constexpr RoadPaintAdditionPiece kStraightSlopeUpAddition0 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kStraightSlopeUp0NE,
            kStraightSlopeUp0SE,
            kStraightSlopeUp0SW,
            kStraightSlopeUp0NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 6, 34 },
            World::Pos3{ 6, 2, 34 },
            World::Pos3{ 2, 6, 34 },
            World::Pos3{ 6, 2, 34 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
        },
        /* Mergable */ false,
        /* Supports */ RoadAdditionSupport{
            /* ImageIds */ std::array<std::array<uint32_t, 4>, 4>{
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
            },
            /* SupportHeight */ 4,
            /* Frequency */ 1,
            /* Segment */ SegmentFlags::x1y0 | SegmentFlags::x1y2,
        },
    };

    // 0x00410BBB, 0x00410D4B, 0x00410EDB, 0x0041106B
    constexpr RoadPaintAdditionPiece kStraightSlopeUpAddition1 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kStraightSlopeUp1NE,
            kStraightSlopeUp1SE,
            kStraightSlopeUp1SW,
            kStraightSlopeUp1NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 6, 34 },
            World::Pos3{ 6, 2, 34 },
            World::Pos3{ 2, 6, 34 },
            World::Pos3{ 6, 2, 34 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
        },
        /* Mergable */ false,
        /* Supports */ RoadAdditionSupport{
            /* ImageIds */ std::array<std::array<uint32_t, 4>, 4>{
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
            },
            /* SupportHeight */ 12,
            /* Frequency */ 1,
            /* Segment */ SegmentFlags::x1y0 | SegmentFlags::x1y2,
        },
    };

    constexpr std::array<RoadPaintAdditionPiece, 2> kStraightSlopeUpTPPA = {
        kStraightSlopeUpAddition0,
        kStraightSlopeUpAddition1,
    };

    constexpr RoadPaintAdditionPiece kStraightSlopeDownAddition0 = rotateRoadPPA(kStraightSlopeUpAddition1, kRotationTable2301);

    constexpr RoadPaintAdditionPiece kStraightSlopeDownAddition1 = rotateRoadPPA(kStraightSlopeUpAddition0, kRotationTable2301);

    constexpr std::array<RoadPaintAdditionPiece, 2> kStraightSlopeDownTPPA = {
        kStraightSlopeDownAddition0,
        kStraightSlopeDownAddition1,
    };

    // 0x00411133, 0x004111FB, 0x004112C3, 0x0041138B
    constexpr RoadPaintAdditionPiece kStraightSteepSlopeUpAddition0 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kStraightSteepSlopeUp0NE,
            kStraightSteepSlopeUp0SE,
            kStraightSteepSlopeUp0SW,
            kStraightSteepSlopeUp0NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 6, 34 },
            World::Pos3{ 6, 2, 34 },
            World::Pos3{ 2, 6, 34 },
            World::Pos3{ 6, 2, 34 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
        },
        /* Mergable */ false,
        /* Supports */ RoadAdditionSupport{
            /* ImageIds */ std::array<std::array<uint32_t, 4>, 4>{
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
                std::array<uint32_t, 4>{ kSupportStraight0NE, kSupportConnectorStraight0NE, kSupportStraight0SW, kSupportConnectorStraight0SW },
                std::array<uint32_t, 4>{ kSupportStraight0SE, kSupportConnectorStraight0SE, kSupportStraight0NW, kSupportConnectorStraight0NW },
            },
            /* SupportHeight */ 8,
            /* Frequency */ 1,
            /* Segment */ SegmentFlags::x1y0 | SegmentFlags::x1y2,
        },
    };

    constexpr std::array<RoadPaintAdditionPiece, 1> kStraightSteepSlopeUpTPPA = {
        kStraightSteepSlopeUpAddition0,
    };

    constexpr RoadPaintAdditionPiece kStraightSteepSlopeDownAddition0 = rotateRoadPPA(kStraightSteepSlopeUpAddition0, kRotationTable2301);

    constexpr std::array<RoadPaintAdditionPiece, 1> kStraightSteepSlopeDownTPPA = {
        kStraightSteepSlopeDownAddition0,
    };

    // 0x004103D7, 0x0041041E, 0x00410465, 0x004104AC
    constexpr RoadPaintAdditionPiece kTurnaroundAddition0 = {
        /* ImageIds */ std::array<uint32_t, 4>{
            kTurnaround0NE,
            kTurnaround0SE,
            kTurnaround0SW,
            kTurnaround0NW,
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 16, 2, 26 },
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 2, 26 },
            World::Pos3{ 2, 16, 26 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 14, 28, 1 },
            World::Pos3{ 28, 14, 1 },
            World::Pos3{ 14, 28, 1 },
            World::Pos3{ 28, 14, 1 },
        },
        /* Mergable */ true,
        /* Supports */ kNoSupports,
    };

    constexpr std::array<RoadPaintAdditionPiece, 1> kTurnaroundTPPA = {
        kTurnaroundAddition0,
    };

    constexpr std::array<std::span<const RoadPaintAdditionPiece>, 10> kRoadPaintAdditionParts = {
        kStraightTPPA,
        kLeftCurveVerySmallTPPA,
        kRightCurveVerySmallTPPA,
        kLeftCurveSmallTPPA,
        kRightCurveSmallTPPA,
        kStraightSlopeUpTPPA,
        kStraightSlopeDownTPPA,
        kStraightSteepSlopeUpTPPA,
        kStraightSteepSlopeDownTPPA,
        kTurnaroundTPPA,
    };
}
