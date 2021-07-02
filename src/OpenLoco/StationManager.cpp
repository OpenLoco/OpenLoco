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

#include <bitset>

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
        enum StationName : uint8_t
        {
            townDefault,
            townNorth,
            townSouth,
            townEast,
            townWest,
            townCentral,
            townTransfer,
            townHalt,
            townValley,
            townHeights,
            townWoods,
            townLakeside,
            townExchange,
            townAirport,
            townOilfield,
            townMines,
            townDocks,
            townAnnexe,
            townSidings,
            townBranch,
            upperTown,
            lowerTown,
            townHeliport,
            townForest,
            townJunction,
            townCross,
            townViews,
            townOrd1,
            townOrd2,
            townOrd3,
            townOrd4,
            townOrd5,
            townOrd6,
            townOrd7,
            townOrd8,
            townOrd9,
            townOrd10,
            townOrd11,
            townOrd12,
            townOrd13,
            townOrd14,
            townOrd15,
            townOrd16,
            townOrd17,
            townOrd18,
            townOrd19,
            townOrd20,
        };

        // Bit mask for station names already used in the current town.
        std::bitset<27> realNamesInUse{};    // ebp
        std::bitset<20> ordinalNamesInUse{}; // edi
        for (auto& station : stations())
        {
            if (!StringManager::isTownName(station.name))
                continue;

            auto nameKey = StringManager::fromTownName(station.name) - StringIds::station_town;
            if (nameKey > StationName::townOrd20)
                continue;

            if (station.town != townId)
                continue;

            if (nameKey < StationName::townOrd1)
                realNamesInUse.set(nameKey, true);
            else
                ordinalNamesInUse.set(nameKey, true);
        }

        if (mode == 1)
        {
            // Airport
            if (!realNamesInUse.test(StationName::townAirport))
                return StringManager::toTownName(StringIds::station_town_airport);
        }
        else if (mode == 2)
        {
            // Heliport
            if (!realNamesInUse.test(StationName::townHeliport))
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

                if (surface->water() == 0)
                {
                    // Docks
                    if (!realNamesInUse.test(StationName::townDocks))
                        return StringManager::toTownName(StringIds::station_town_docks);
                }
                break;
            }
        }

        // 0x0048FA41
        if (IndustryManager::industryNearPosition(Map::Pos2(position.x, position.y), IndustryObjectFlags::oilfield))
        {
            if (!realNamesInUse.test(StationName::townOilfield))
                return StringManager::toTownName(StringIds::station_town_oilfield);
        }

        if (IndustryManager::industryNearPosition(Map::Pos2(position.x, position.y), IndustryObjectFlags::mines))
        {
            if (!realNamesInUse.test(StationName::townMines))
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
                    if (!realNamesInUse.test(StationName::townLakeside))
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
            if (!realNamesInUse.test(StationName::townForest))
                return StringManager::toTownName(StringIds::station_town_forest);
        }
        else if (numSurroundingTrees > 20)
        {
            // Woods
            if (!realNamesInUse.test(StationName::townWoods))
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
                    if (!realNamesInUse.test(StationName::townHeights))
                        return StringManager::toTownName(StringIds::station_town_heights);
                }
                else if (townHeightDiff < -20)
                {
                    // Valley
                    if (!realNamesInUse.test(StationName::townValley))
                        return StringManager::toTownName(StringIds::station_town_valley);
                }
                break;
            }
        }

        // 0x0048FB8B
        if (!realNamesInUse.test(StationName::townDefault))
            return StringManager::toTownName(StringIds::station_town);

        auto town = TownManager::get(townId);
        {
            auto manhattanDistance = Math::Vector::manhattanDistance(position, Map::Pos2{ town->x, town->y });
            if (manhattanDistance / Map::tile_size <= 9)
            {
                // Central
                if (!realNamesInUse.test(StationName::townCentral))
                    return StringManager::toTownName(StringIds::station_town_central);
            }
        }

        if (position.x <= town->x && position.y <= town->y)
        {
            // North
            if (!realNamesInUse.test(StationName::townNorth))
                return StringManager::toTownName(StringIds::station_town_north);
        }

        if (position.x >= town->x && position.y >= town->y)
        {
            // South
            if (!realNamesInUse.test(StationName::townSouth))
                return StringManager::toTownName(StringIds::station_town_south);
        }

        if (position.x <= town->x && position.y >= town->y)
        {
            // East
            if (!realNamesInUse.test(StationName::townEast))
                return StringManager::toTownName(StringIds::station_town_east);
        }

        if (position.x >= town->x && position.y <= town->y)
        {
            // West
            if (!realNamesInUse.test(StationName::townWest))
                return StringManager::toTownName(StringIds::station_town_west);
        }

        // Additional names to try
        static const std::pair<const StationName, const string_id> additionalNamePairs[] = {
            { StationName::townTransfer, StringIds::station_town_transfer },
            { StationName::townHalt, StringIds::station_town_halt },
            { StationName::townAnnexe, StringIds::station_town_annexe },
            { StationName::townSidings, StringIds::station_town_sidings },
            { StationName::townBranch, StringIds::station_town_branch },
            { StationName::townJunction, StringIds::station_town_junction },
            { StationName::townCross, StringIds::station_town_cross },
            { StationName::townViews, StringIds::station_town_views },
        };

        for (auto namePair : additionalNamePairs)
        {
            if (!realNamesInUse.test(namePair.first))
                return StringManager::toTownName(namePair.second);
        }

        // Ordinal names to try
        static const std::pair<const StationName, const string_id> ordinalNamePairs[] = {
            { StationName::townOrd1, StringIds::station_town_ord_1 },
            { StationName::townOrd2, StringIds::station_town_ord_2 },
            { StationName::townOrd3, StringIds::station_town_ord_3 },
            { StationName::townOrd4, StringIds::station_town_ord_4 },
            { StationName::townOrd5, StringIds::station_town_ord_5 },
            { StationName::townOrd6, StringIds::station_town_ord_6 },
            { StationName::townOrd7, StringIds::station_town_ord_7 },
            { StationName::townOrd8, StringIds::station_town_ord_8 },
            { StationName::townOrd9, StringIds::station_town_ord_9 },
            { StationName::townOrd10, StringIds::station_town_ord_10 },
            { StationName::townOrd11, StringIds::station_town_ord_11 },
            { StationName::townOrd12, StringIds::station_town_ord_12 },
            { StationName::townOrd13, StringIds::station_town_ord_13 },
            { StationName::townOrd14, StringIds::station_town_ord_14 },
            { StationName::townOrd15, StringIds::station_town_ord_15 },
            { StationName::townOrd16, StringIds::station_town_ord_16 },
            { StationName::townOrd17, StringIds::station_town_ord_17 },
            { StationName::townOrd18, StringIds::station_town_ord_18 },
            { StationName::townOrd19, StringIds::station_town_ord_19 },
            { StationName::townOrd20, StringIds::station_town_ord_20 },
        };

        for (auto namePair : ordinalNamePairs)
        {
            if (!ordinalNamesInUse.test(namePair.first))
                return StringManager::toTownName(namePair.second);
        }

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
                auto stationId = (reinterpret_cast<Station*>(regs.esi))->id();
                regs.bx = generateNewStationName(stationId, regs.ebx, Map::Pos3(regs.ax, regs.cx, regs.dh), regs.dl);
                return 0;
            });
    }
}
