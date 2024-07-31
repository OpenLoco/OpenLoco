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

    struct RoadConnections
    {
        sfl::static_vector<uint16_t, 16> connections;
        StationId stationId = StationId::null; // 0x01135FAE
        uint8_t stationObjectId = 0U;          // 0x01136087
        uint8_t roadObjectId = 0U;             // 0x0112C2ED (I wouldn't trust this to be correct which connection!)
    };

    // requiredMods : 0x0113601A
    // queryMods : 0x0113601B
    //
    // if set requiredMods must exist on connections to be added to connection list
    //  e.g. if required mods is `0b10` then tracks with mods `0b10` and `0b11` can connect
    //  but `0b00` and `0b01` cannot
    // if requiredMods == 0 then mods ignored
    //
    // queryMods sets AdditionalTaDFlags::hasMods of connection if connection has the queryMods
    RoadConnections getRoadConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t roadObjectId, const uint8_t requiredMods, const uint8_t queryMods);

    struct ConnectionEnd
    {
        World::Pos3 nextPos;
        uint8_t nextRotation;
    };

    ConnectionEnd getRoadConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection);

    struct TrackConnections
    {
        sfl::static_vector<uint16_t, 16> connections;
        bool hasLevelCrossing = false;         // 0x0113607D
        StationId stationId = StationId::null; // 0x01135FAE
    };
    void toLegacyConnections(const TrackConnections& src, LegacyTrackConnections& data);
    void toLegacyConnections(const RoadConnections& src, LegacyTrackConnections& data);

    // requiredMods : 0x0113601A
    // queryMods : 0x0113601B
    //
    // if set requiredMods must exist on connections to be added to connection list
    //  e.g. if required mods is `0b10` then tracks with mods `0b10` and `0b11` can connect
    //  but `0b00` and `0b01` cannot
    // if requiredMods == 0 then mods ignored
    //
    // queryMods sets AdditionalTaDFlags::hasMods of connection if connection has the queryMods
    TrackConnections getTrackConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t trackObjectId, const uint8_t requiredMods, const uint8_t queryMods);
    TrackConnections getTrackConnectionsAi(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t trackObjectId, const uint8_t requiredMods, const uint8_t queryMods);
    ConnectionEnd getTrackConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection);
}
