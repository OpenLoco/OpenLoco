#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Station.h"
#include <array>
#include <cstddef>

namespace OpenLoco::StationManager
{
    constexpr size_t max_stations = 1024;

    void reset();
    LocoFixedVector<Station> stations();
    Station* get(StationId_t id);
    void update();
    void updateLabels();
    void updateDaily();
    string_id generateNewStationName(StationId_t stationId, TownId_t townId, Map::Pos3 position, uint8_t mode);
    void zeroUnused();
    void registerHooks();
    uint16_t sendProducedCargoToStations(const uint8_t cargoType, const uint8_t cargoQty, const Map::Pos2& pos, const Map::TilePos2& size);
}
