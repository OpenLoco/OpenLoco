#pragma once

#include "station.h"
#include <cstddef>

namespace openloco::stationmgr
{
    constexpr size_t max_stations = 1024;

    station* get(station_id_t id);
    void sub_48B244();
}
