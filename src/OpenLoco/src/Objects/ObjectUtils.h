#pragma once

#include "Types.hpp"
#include <sfl/static_vector.hpp>

namespace OpenLoco
{
    enum class TransportMode : uint8_t;

    sfl::static_vector<uint8_t, 16> getAvailableAirports();
    sfl::static_vector<uint8_t, 16> getAvailableDocks();
    sfl::static_vector<uint8_t, 16> getAvailableCompatibleStations(uint8_t trackType, TransportMode transportMode);
    sfl::static_vector<uint8_t, 8> getAvailableCompatibleBridges(uint8_t trackType, TransportMode transportMode);
    std::array<uint8_t, 4> getAvailableCompatibleMods(uint8_t trackType, TransportMode transportMode, CompanyId companyId);
    sfl::static_vector<uint8_t, 16> getAvailableCompatibleSignals(uint8_t trackType);
}
