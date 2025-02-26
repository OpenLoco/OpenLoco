#pragma once

#include "ObjectManager.h"
#include "Types.hpp"
#include <sfl/static_vector.hpp>

namespace OpenLoco
{
    enum class TransportMode : uint8_t;

    sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::airport)> getAvailableAirports();
    sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::dock)> getAvailableDocks();

    sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::trainStation)> getAvailableCompatibleStations(uint8_t trackType, TransportMode transportMode);
    static_assert(ObjectManager::getMaxObjects(ObjectType::roadStation) <= ObjectManager::getMaxObjects(ObjectType::trainStation));

    sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::bridge)> getAvailableCompatibleBridges(uint8_t trackType, TransportMode transportMode);

    static constexpr uint8_t kMaxMods = 4; // road its only 2 but we use 4 for both road and rail
    std::array<uint8_t, kMaxMods> getAvailableCompatibleMods(uint8_t trackType, TransportMode transportMode, CompanyId companyId);
    sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::trackSignal)> getAvailableCompatibleSignals(uint8_t trackType);
}
