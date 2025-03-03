#pragma once

#include "Engine/Limits.h"
#include "Types.hpp"
#include <sfl/static_vector.hpp>

namespace OpenLoco
{
    enum class TransportMode : uint8_t;

    sfl::static_vector<uint8_t, Limits::kMaxAirportObjects> getAvailableAirports();
    sfl::static_vector<uint8_t, Limits::kMaxDockObjects> getAvailableDocks();

    sfl::static_vector<uint8_t, Limits::kMaxTrainStationObjects> getAvailableCompatibleStations(uint8_t trackType, TransportMode transportMode);
    static_assert(Limits::kMaxRoadStationObjects <= Limits::kMaxTrainStationObjects);

    sfl::static_vector<uint8_t, Limits::kMaxBridgeObjects> getAvailableCompatibleBridges(uint8_t trackType, TransportMode transportMode);

    static constexpr uint8_t kMaxMods = 4; // road its only 2 but we use 4 for both road and rail
    std::array<uint8_t, kMaxMods> getAvailableCompatibleMods(uint8_t trackType, TransportMode transportMode, CompanyId companyId);
    sfl::static_vector<uint8_t, Limits::kMaxTrainSignalObjects> getAvailableCompatibleSignals(uint8_t trackType);
}
