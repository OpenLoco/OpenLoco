#pragma once

#include "Map/Map.hpp"
#include "Types.hpp"
#include <utility>

namespace OpenLoco::Map::Track
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

    void getRoadConnections(const Map::Pos3& pos, TrackConnections& data, const CompanyId company, const uint8_t roadObjectId, const uint16_t trackAndDirection);
    void getTrackConnections(const Map::Pos3& nextTrackPos, const uint8_t nextRotation, TrackConnections& data, const CompanyId company, const uint8_t trackObjectId);
    std::pair<Map::Pos3, uint8_t> getTrackConnectionEnd(const Map::Pos3& pos, const uint16_t trackAndDirection);
}
