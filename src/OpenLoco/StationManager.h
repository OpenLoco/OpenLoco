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
}
