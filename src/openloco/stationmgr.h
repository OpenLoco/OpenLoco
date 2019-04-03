#pragma once

#include "station.h"
#include <array>
#include <cstddef>

namespace openloco::stationmgr
{
    constexpr size_t max_stations = 1024;

    std::array<station, max_stations>& stations();
    station* get(station_id_t id);
    void update();
    void update_labels();
    void update_daily();
}
