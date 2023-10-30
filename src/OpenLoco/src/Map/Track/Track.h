#pragma once

#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>
#include <utility>

namespace OpenLoco::World::Track
{
    struct TrackConnections
    {
        uint32_t size;
        uint16_t data[16];
        void push_back(uint16_t value);
        uint16_t pop_back()
        {
            return data[--size];
        }
    };
    static_assert(sizeof(TrackConnections) == 0x24);

    namespace AdditionalTaDFlags
    {
        constexpr uint16_t basicTaDMask = 0b0000'0001'1111'1111;
        constexpr uint16_t bridgeMask = 0b0000'1110'0000'0000;
        constexpr uint16_t hasBridge = (1U << 12);
        constexpr uint16_t hasMods = (1U << 13);
        constexpr uint16_t hasSignal = (1U << 15); // Track only (not roads)
        constexpr uint16_t basicTaDWithSignalMask = basicTaDMask | hasSignal;
    }

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

    void getRoadConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, TrackConnections& data, const CompanyId company, const uint8_t roadObjectId);
    std::pair<World::Pos3, uint8_t> getRoadConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection);
    void getTrackConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, TrackConnections& data, const CompanyId company, const uint8_t trackObjectId);
    std::pair<World::Pos3, uint8_t> getTrackConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection);
}
