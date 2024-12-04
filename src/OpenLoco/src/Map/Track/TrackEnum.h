#pragma once

#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstdint>

namespace OpenLoco::World::Track
{
    enum class TrackId : uint8_t
    {
        straight,
        diagonal,
        leftCurveVerySmall,
        rightCurveVerySmall,
        leftCurveSmall,
        rightCurveSmall,
        leftCurve,
        rightCurve,
        leftCurveLarge,
        rightCurveLarge,
        diagonalLeftCurveLarge,
        diagonalRightCurveLarge,
        sBendLeft,
        sBendRight,
        straightSlopeUp,
        straightSlopeDown,
        straightSteepSlopeUp,
        straightSteepSlopeDown,
        leftCurveSmallSlopeUp,
        rightCurveSmallSlopeUp,
        leftCurveSmallSlopeDown,
        rightCurveSmallSlopeDown,
        leftCurveSmallSteepSlopeUp,
        rightCurveSmallSteepSlopeUp,
        leftCurveSmallSteepSlopeDown,
        rightCurveSmallSteepSlopeDown,
        unkStraight1,
        unkStraight2,
        unkLeftCurveVerySmall1,
        unkLeftCurveVerySmall2,
        unkRightCurveVerySmall1,
        unkRightCurveVerySmall2,
        unkSBendRight,
        unkSBendLeft,
        unkStraightSteepSlopeUp1,
        unkStraightSteepSlopeUp2,
        unkStraightSteepSlopeDown1,
        unkStraightSteepSlopeDown2,
        sBendToDualTrack,
        sBendToSingleTrack,
        unkSBendToDualTrack,
        unkSBendToSingleTrack,
        turnaround,
        unkTurnaround,
    };

    enum class RoadId : uint8_t
    {
        straight,
        leftCurveVerySmall,
        rightCurveVerySmall,
        leftCurveSmall,
        rightCurveSmall,
        straightSlopeUp,
        straightSlopeDown,
        straightSteepSlopeUp,
        straightSteepSlopeDown,
        turnaround,
    };

    // For some reason we have Common, Track and Road Trait flags
    // but they are all pretty much the same thing. One day we should
    // look into amalgamating them into one.

    enum class CommonTraitFlags : uint16_t
    {
        none = 0U,
        slope = 1U << 0,
        steepSlope = 1U << 1,
        curveSlope = 1U << 2,
        diagonal = 1U << 3,
        verySmallCurve = 1U << 4,
        smallCurve = 1U << 5,
        curve = 1U << 6,
        largeCurve = 1U << 7,
        sBendCurve = 1U << 8,
        oneSided = 1U << 9,
        startsAtHalfHeight = 1U << 10, // Not used. From RCT2
        junction = 1U << 11,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(CommonTraitFlags);

    enum class TrackTraitFlags : uint16_t
    {
        none = 0U,
        diagonal = 1U << 0,
        largeCurve = 1U << 1,
        normalCurve = 1U << 2,
        smallCurve = 1U << 3,
        verySmallCurve = 1U << 4,
        slope = 1U << 5,
        steepSlope = 1U << 6,
        oneSided = 1U << 7,
        slopedCurve = 1U << 8,
        sBend = 1U << 9,
        junction = 1U << 10,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TrackTraitFlags);

    enum class RoadTraitFlags : uint16_t
    {
        none = 0U,
        smallCurve = 1U << 0,
        verySmallCurve = 1U << 1,
        slope = 1U << 2,
        steepSlope = 1U << 3,

        turnaround = 1U << 5,
        junction = 1U << 6,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadTraitFlags);
}
