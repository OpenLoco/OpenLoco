#include "StationManager.h"
#include "CompanyManager.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
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
        // Bit mask for station names already used in the current town.
        uint32_t realNamesInUse{};    // ebp
        uint32_t ordinalNamesInUse{}; // edi
        for (auto& station : stations())
        {
            if (station.empty())
                continue;

            if (!StringManager::isTownName(station.name))
                continue;

            auto nameKey = StringManager::fromTownName(station.name) - StringIds::station_town;
            if (nameKey >= 47)
                continue;

            if (station.town != townId)
                continue;

            if (nameKey < 27)
                realNamesInUse |= (1 << nameKey);
            else
                ordinalNamesInUse |= (1 << (nameKey - 27));
        }

        if (mode == 1)
        {
            // Airport
            if ((realNamesInUse & (1 << 13)) == 0)
                return StringManager::toTownName(StringIds::station_town_airport);
        }
        else if (mode == 2)
        {
            // Heliport
            if ((realNamesInUse & (1 << 22)) == 0)
                return StringManager::toTownName(StringIds::station_town_heliport);
        }
        else if (mode == 3)
        {
            // 0x0048FA00
            auto tile = TileManager::get(Map::Pos2(position.x, position.y));
            for (auto& element : tile)
            {
                auto* surface = element.asSurface();
                if (surface == nullptr)
                    continue;

                if (surface->water() > 0)
                {
                    // Docks
                    if ((realNamesInUse & (1 << 16)) == 0)
                        return StringManager::toTownName(StringIds::station_town_docks);
                }
                break;
            }
        }

        // 0x0048FA41
        if (IndustryManager::industryExistsAtPosition(Map::Pos2(position.x, position.y), IndustryObjectFlags::oilfield))
        {
            if ((realNamesInUse & (1 << 14)) == 0)
                return StringManager::toTownName(StringIds::station_town_oilfield);
        }

        if (IndustryManager::industryExistsAtPosition(Map::Pos2(position.x, position.y), IndustryObjectFlags::mines))
        {
            if ((realNamesInUse & (1 << 15)) == 0)
                return StringManager::toTownName(StringIds::station_town_mines);
        }

        // 0x0048FA91
        auto numSurroundingWaterTiles = TileManager::countSurroundingWaterTiles(Map::Pos2(position.x, position.y));
        if (numSurroundingWaterTiles >= 24)
        {
            auto tile = TileManager::get(Map::Pos2(position.x, position.y));
            for (auto& element : tile)
            {
                auto* surface = element.asSurface();
                if (surface == nullptr)
                    continue;

                if (surface->water() == 0)
                {
                    // Lakeside
                    if ((realNamesInUse & (1 << 11)) == 0)
                        return StringManager::toTownName(StringIds::station_town_lakeside);
                }
                break;
            }
        }

        // 0x0048FAEB
        auto numSurroundingTrees = TileManager::countSurroundingTrees(Map::Pos2(position.x, position.y));
        if (numSurroundingTrees > 40)
        {
            // Forest
            if ((realNamesInUse & (1 << 23)) == 0)
                return StringManager::toTownName(StringIds::station_town_forest);
        }
        else if (numSurroundingTrees > 20)
        {
            // Woods
            if ((realNamesInUse & (1 << 10)) == 0)
                return StringManager::toTownName(StringIds::station_town_woods);
        }

        // 0x0048FB29
        {
            auto tile = TileManager::get(Map::Pos2(position.x, position.y));
            for (auto& element : tile)
            {
                auto* surface = element.asSurface();
                if (surface == nullptr)
                    continue;

                auto townHeightDiff = (position.z / 4) - surface->baseZ();
                if (townHeightDiff > 20)
                {
                    // Heights
                    if ((realNamesInUse & (1 << 9)) == 0)
                        return StringManager::toTownName(StringIds::station_town_heights);
                }
                else if (townHeightDiff < -20)
                {
                    // Valley
                    if ((realNamesInUse & (1 << 8)) == 0)
                        return StringManager::toTownName(StringIds::station_town_valley);
                }
                break;
            }
        }

        // 0x0048FB8B
        if ((realNamesInUse & (1 << 0)) == 0)
            return StringManager::toTownName(StringIds::station_town);

        auto town = TownManager::get(townId);
        {
            auto xDiff = std::abs(position.x - town->x);
            auto yDiff = std::abs(position.y - town->y);
            if (xDiff + yDiff <= 120)
            {
                // Central
                if ((realNamesInUse & (1 << 5)) == 0)
                    return StringManager::toTownName(StringIds::station_town_central);
            }
        }

        if (position.x <= town->x && position.y <= town->y)
        {
            // North
            if ((realNamesInUse & (1 << 1)) == 0)
                return StringManager::toTownName(StringIds::station_town_north);
        }

        if (position.x >= town->x && position.y >= town->y)
        {
            // South
            if ((realNamesInUse & (1 << 2)) == 0)
                return StringManager::toTownName(StringIds::station_town_south);
        }

        if (position.x <= town->x && position.y >= town->y)
        {
            // East
            if ((realNamesInUse & (1 << 3)) == 0)
                return StringManager::toTownName(StringIds::station_town_east);
        }

        if (position.x >= town->x && position.y <= town->y)
        {
            // West
            if ((realNamesInUse & (1 << 4)) == 0)
                return StringManager::toTownName(StringIds::station_town_west);
        }

        // 0x0048FC5C
        string_id foundName = StringIds::empty;
        for (auto i = 5; i < 47; i++)
        {
            // At several positions, skip the entries we've already tested for earlier.
            if (i == 8)
                i = 12;
            else if (i == 13)
                i = 17;
            else if (i == 22)
                i = 24;

            bool realNameIsInUse = (i < 27 && (realNamesInUse & (1 << i)) != 0);
            bool ordinalNameIsInUse = (i >= 27 && (ordinalNamesInUse & (1 << (i - 27))) != 0);
            if (!(realNameIsInUse || ordinalNameIsInUse))
            {
                foundName = StringManager::toTownName(StringIds::station_town + i);
                break;
            }
        }

        if (foundName != StringIds::empty)
            return foundName;

        // Default to an ordinal string instead, e.g. 'Station 42'.
        char stationName[256] = "";
        auto args = FormatArguments::common(stationId);
        StringManager::formatString(stationName, StringIds::station_name_ordinal, &args);
        return StringManager::userStringAllocate(stationName, 0);
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

    void registerHooks()
    {
        // Can be removed once the createStation function has been implemented (used by place.*Station game commands)
        registerHook(
            0x048F988,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                StationId_t stationId = static_cast<StationId_t>((0x005E6EDC - regs.esi) / sizeof(Station));
                regs.bx = generateNewStationName(stationId, regs.ebx, Map::Pos3(regs.ax, regs.cx, regs.dh), regs.dl);
                return 0;
            });
    }
}
