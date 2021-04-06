#pragma once

#include "Station.h"
#include <array>
#include <cstddef>

namespace OpenLoco::StationManager
{
    constexpr size_t max_stations = 1024;

    void reset();
    std::array<Station, max_stations>& stations();
    Station* get(StationId_t id);
    void update();
    void updateLabels();
    void updateDaily();
    void zeroUnused();
}
