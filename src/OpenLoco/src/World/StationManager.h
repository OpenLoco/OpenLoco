#pragma once

#include "Engine/Limits.h"
#include "Station.h"
#include <OpenLoco/Core/LocoFixedVector.hpp>
#include <array>
#include <cstddef>
#include <span>

namespace OpenLoco::StationManager
{
    // If it exceeds this distance, it will not be considered a nearby station
    constexpr int16_t kMaxStationNearbyDistance = 64;

    void reset();
    FixedVector<Station, Limits::kMaxStations> stations();
    Station* get(StationId id);
    void update();
    void updateLabels();
    void updateDaily();
    StringId generateNewStationName(StationId stationId, TownId townId, World::Pos3 position, uint8_t mode);
    void zeroUnused();
    uint16_t deliverCargoToNearbyStations(const uint8_t cargoType, const uint8_t cargoQty, const World::Pos2& pos, const World::TilePos2& size);
    uint16_t deliverCargoToStations(std::span<const StationId> stations, const uint8_t cargoType, const uint8_t cargoQty);
    bool exceedsStationSize(Station& station, World::Pos3 pos);
    StationId allocateNewStation(const World::Pos3 pos, const CompanyId owner, const uint8_t mode);
    void deallocateStation(const StationId stationId);

    struct NearbyStation
    {
        StationId id;
        bool isPhysicallyAttached;
    };

    NearbyStation findNearbyStation(World::Pos3 pos, CompanyId companyId);
    // Subfunction of findNearbyStation (For create airport)
    StationId findNearbyEmptyStation(const World::Pos3 pos, const CompanyId companyId, const int16_t currentMinDistanceStation);
}
