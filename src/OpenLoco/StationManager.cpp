#include "StationManager.h"
#include "CompanyManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "OpenLoco.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::StationManager
{
    static loco_global<Station[max_stations], 0x005E6EDC> _stations;

    // 0x0048B1D8
    void reset()
    {
        call(0x0048B1D8);
    }

    LocoFixedVector<Station> stations()
    {
        return LocoFixedVector<Station>(_stations);
    }

    Station* get(StationId_t id)
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
        if ((addr<0x00525E28, uint32_t>() & 1) && !isEditorMode())
        {
            StationId_t id = scenarioTicks() & 0x3FF;
            auto station = get(id);
            if (station != nullptr && !station->empty())
            {
                station->update();
            }
        }
    }

    // 0x0048DDC3
    void updateLabels()
    {
        for (auto& station : stations())
        {
            station.updateLabel();
        }
    }

    // 0x00437F29
    // arg0: ah
    // arg1: al
    static void sub_437F29(CompanyId_t cid, uint8_t arg1)
    {
        constexpr uint8_t byte_4F9462[] = { 0, 31, 10, 7, 31, 10, 31, 31, 11 };
        auto company = CompanyManager::get(cid);
        company->var_8BB0[arg1] = byte_4F9462[arg1];
    }

    static void sub_49E1F1(StationId_t id)
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
    void updateDaily()
    {
        for (auto& town : TownManager::towns())
        {
            town.flags &= ~TownFlags::ratingAdjusted;
        }

        for (auto& station : stations())
        {
            if (station.stationTileSize == 0)
            {
                station.var_29++;
                if (station.var_29 != 5 && isPlayerCompany(station.owner))
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
            if (station.updateCargo())
            {
                auto town = TownManager::get(station.town);
                if (town != nullptr && !(town->flags & TownFlags::ratingAdjusted))
                {
                    town->flags |= TownFlags::ratingAdjusted;
                    town->adjustCompanyRating(station.owner, 1);
                }
            }
        }
    }

    // 0x048F988
    string_id generateNewStationName(StationId_t stationId, TownId_t townId, Map::Pos3 position, uint8_t mode)
    {
        auto* station = get(stationId);
        if (station == nullptr)
            return StringIds::null;

        station->name = StringIds::null;

        registers regs;
        regs.esi = reinterpret_cast<int32_t>(station);
        regs.ebx = townId;
        regs.dh = static_cast<uint8_t>(position.z / 4);
        regs.dl = mode;
        regs.ax = position.x & 0xFFE0;
        regs.cx = position.y & 0xFFE0;

        call(0x048F988, regs);
        return regs.bx;
    }

    // 0x0049088B
    void zeroUnused()
    {
        for (auto& station : _stations)
        {
            if (station.empty())
            {
                // Zero unused station
                station = {};
            }
            else
            {
                // Zero unused station tiles
                for (auto i = station.stationTileSize; i < std::size(station.stationTiles); i++)
                {
                    station.stationTiles[i] = {};
                }
            }
        }
    }
}
