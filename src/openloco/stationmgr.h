#pragma once

#include "station.h"
#include <cstdint>

namespace openloco::stationmgr
{
    station* get(station_id_t id);
}
