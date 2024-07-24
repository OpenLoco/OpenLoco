#pragma once

#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>
#include <sfl/static_vector.hpp>
#include <utility>

namespace OpenLoco::World::Track
{
    struct LegacyTrackConnections
    {
        uint32_t size;
        uint16_t data[16];
        void push_back(uint16_t value);
        uint16_t pop_back()
        {
            return data[--size];
        }
    };
    static_assert(sizeof(LegacyTrackConnections) == 0x24);

    namespace AdditionalTaDFlags
    {
        constexpr uint16_t basicTaDMask = 0b0000'0001'1111'1111;
        constexpr uint16_t bridgeMask = 0b0000'1110'0000'0000;
        constexpr uint16_t hasBridge = (1U << 12);
        constexpr uint16_t hasMods = (1U << 13);
        constexpr uint16_t hasSignal = (1U << 15); // Track only (not roads)
        constexpr uint16_t basicTaDWithSignalMask = basicTaDMask | hasSignal;
    }

    void getRoadConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, LegacyTrackConnections& data, const CompanyId company, const uint8_t roadObjectId);

    struct ConnectionEnd
    {
        World::Pos3 nextPos;
        uint8_t nextRotation;
    };

    ConnectionEnd getRoadConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection);

    struct TrackConnections
    {
        sfl::static_vector<uint16_t, 16> connections;
        bool hasLevelCrossing = false;
        StationId stationId = StationId::null;
    };
    void toLegacyConnections(const TrackConnections& src, LegacyTrackConnections& data);

    TrackConnections getTrackConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t trackObjectId);
    ConnectionEnd getTrackConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection);
}
