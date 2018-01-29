#include "stationmgr.h"
#include "companymgr.h"
#include "interop/interop.hpp"
#include "townmgr.h"
#include "window.h"
#include "windowmgr.h"

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::stationmgr
{
    static loco_global<station[max_stations], 0x005E6EDC> _stations;

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
    static void sub_437F29(company_id_t cid, uint8_t arg1)
    {
        constexpr uint8_t byte_4F9462[] = { 0, 31, 10, 7, 31, 10, 31, 31, 11 };
        auto company = companymgr::get(cid);
        company->var_8BB0[arg1] = byte_4F9462[arg1];
    }

    static void sub_49E1F1(station_id_t id)
    {
        auto w = windowmgr::find(window_type::construction);
        if (w != nullptr && w->var_870 == 1)
        {
            if ((addr<0x00522096, uint8_t>() & 8) && addr<0x01135F70, int32_t>() == id)
            {
                addr<0x01135F70, int32_t>() = -1;
                w->invalidate();
            }
        }
    }

    // 0x0048B244
    void sub_48B244()
    {
        for (auto& town : townmgr::towns())
        {
            if (!town.empty())
            {
                town.flags &= ~town_flags::rating_adjusted;
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
                        sub_49E1F1(station.id());
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
                    if (town != nullptr && !(town->flags & town_flags::rating_adjusted))
                    {
                        town->flags |= town_flags::rating_adjusted;
                        town->adjust_company_rating(station.company, 1);
                    }
                }
            }
        }
    }
}
