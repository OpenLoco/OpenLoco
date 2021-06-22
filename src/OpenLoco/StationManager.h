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
    void zeroUnused();
}
