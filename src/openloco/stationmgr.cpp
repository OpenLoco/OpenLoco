#include "stationmgr.h"
#include "company.h"
#include "interop/interop.hpp"
#include "townmgr.h"
#include "window.h"
#include <array>

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::stationmgr
{
    static loco_global_array<station, max_stations, 0x005E6EDC> _stations;

    std::array<station, max_stations>& stations()
    {
        auto arr = (std::array<station, max_stations>*)_stations.get();
        return *arr;
    }

    station* get(station_id_t id)
    {
        auto index = (size_t)id;
        if (index < _stations.size())
        {
            return &_stations[index];
        }
        return nullptr;
    }

    // 0x00437F29
    // arg0: ah
    // arg1: al
    static void sub_437F29(uint8_t arg0, uint8_t arg1)
    {
        constexpr uint8_t byte_4F9462[] = { 0, 31, 10, 7, 31, 10, 31, 31, 11 };
        static loco_global_array<uint8_t, 9, 0x0053A334> byte_53A334;
        byte_53A334[(arg0 * 36776) + arg1] = byte_4F9462[arg1];
    }

    // 0x0048B244
    void sub_48B244()
    {
        for (auto& town : townmgr::towns())
        {
            if (!town.empty())
            {
                town.var_06 &= town_flags::flag_1;
            }
        }

        for (auto& station : stations())
        {
            if (!station.empty())
            {
                if (station.var_1CE == 0)
                {
                    station.var_29++;
                    if (station.var_29 != 5 && is_player_company(station.company))
                    {
                        sub_437F29(station.company, 8);
                    }
                    if (station.var_29 >= 10)
                    {
                        call(0x0049E1F1);
                        station.invalidate();
                        station.sub_48F7D1();
                    }
                }
                else
                {
                    station.var_29 = 0;
                }
                if (station.update_cargo())
                {
                    auto town = townmgr::get(station.town);
                    if (town != nullptr && !(town->var_06 & town_flags::flag_1))
                    {
                        town->var_06 |= town_flags::flag_1;
                        town->adjust_company_rating(station.company, 1);
                    }
                }
            }
        }
    }
}
