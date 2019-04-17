#include "stationmgr.h"
#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include "townmgr.h"
#include "ui/WindowManager.h"
#include "window.h"

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

    // 0x0048B1FA
    void update()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
        {
            station_id_t id = scenario_ticks() & 0x3FF;
            auto station = get(id);
            if (station != nullptr && !station->empty())
            {
                station->update();
            }
        }
    }

    // 0x0048DDC3
    void update_labels()
    {
        call(0x0048DDC3);
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
        auto w = WindowManager::find(WindowType::construction);
        if (w != nullptr && w->current_tab == 1)
        {
            if ((addr<0x00522096, uint8_t>() & 8) && addr<0x01135F70, int32_t>() == id)
            {
                addr<0x01135F70, int32_t>() = -1;
                w->invalidate();
            }
        }
    }

    // 0x0048B244
    void update_daily()
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
                    if (station.var_29 != 5 && is_player_company(station.owner))
                    {
                        sub_437F29(station.owner, 8);
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
                        town->adjust_company_rating(station.owner, 1);
                    }
                }
            }
        }
    }
}
