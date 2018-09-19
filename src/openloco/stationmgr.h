#pragma once

#include "company.h"
#include "station.h"
#include <array>
#include <cstddef>

namespace openloco
{
    class companymanager;
    class townmanager;
}

namespace openloco
{
    constexpr size_t max_stations = 1024;

    class stationmanager
    {
    public:
        std::array<station, max_stations>& stations();
        station* get(station_id_t id);
        void update(companymanager& companymgr, messagemanager& messagemgr);
        void update_daily(companymanager& companymgr, townmanager& townmgr);

    private:
        void sub_437F29(companymanager& companymgr, company_id_t cid, uint8_t arg1);
        void sub_49E1F1(station_id_t id);
    };

    extern stationmanager g_stationmgr;
}
