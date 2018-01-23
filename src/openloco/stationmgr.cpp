#include "stationmgr.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::stationmgr
{
    static loco_global_array<station, 1024, 0x005E6EDC> _stations;

    station* get(station_id_t id)
    {
        auto index = (size_t)id;
        if (index < _stations.size())
        {
            return &_stations[index];
        }
        return nullptr;
    }
}
