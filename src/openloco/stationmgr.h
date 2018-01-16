#pragma once

#include <cstdint>
#include "station.h"

namespace openloco::stationmgr
{
    station * get(station_id_t id);
}
