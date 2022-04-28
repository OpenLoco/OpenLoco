#include "StationManager.h"
#include "CompanyManager.h"
#include "Game.h"
#include "GameState.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "OpenLoco.h"
#include "ScenarioManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Window.h"

#include <bitset>
#include <numeric>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Map;

namespace OpenLoco::StationManager
{
    static auto& rawStations() { return getGameState().stations; }

    // 0x0048B1D8
    void reset()
    {
        for (auto& station : rawStations())
        {
            station.name = StringIds::null;
        }
        Ui::Windows::Station::reset();
    }

    FixedVector<Station, Limits::kMaxStations> stations()
    {
        return FixedVector(rawStations());
    }

    Station* get(StationId id)
    {
        auto index = (size_t)id;
        if (index < Limits::kMaxStations)
        {
            return &rawStations()[index];
        }
        return nullptr;
    }

    // 0x0048B1FA
    void update()
    {
        if (Game::hasFlags(1u << 0) && !isEditorMode())
        {
            const auto id = StationId(ScenarioManager::getScenarioTicks() & 0x3FF);
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
    void sub_437F29(CompanyId cid, uint8_t arg1)
    {
        constexpr uint8_t byte_4F9462[] = { 0, 31, 10, 7, 31, 10, 31, 31, 11 };
        auto company = CompanyManager::get(cid);
        company->var_8BB0[arg1] = byte_4F9462[arg1];
    }

    static void sub_49E1F1(StationId id)
    {
        auto w = WindowManager::find(WindowType::construction);
        if (w != nullptr && w->currentTab == 1)
        {
            if ((addr<0x00522096, uint8_t>() & 8) && StationId(addr<0x01135F70, int32_t>()) == id) // _constructingStationId
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
                if (station.var_29 != 5 && CompanyManager::isPlayerCompany(station.owner))
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
    string_id generateNewStationName(StationId stationId, TownId townId, Map::Pos3 position, uint8_t mode)
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
                ordinalNamesInUse.set(nameKey - StationName::townOrd1, true);
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
            auto* surface = tile.surface();
            if (surface != nullptr && surface->water() == 0)
            {
                // Docks
                if (!realNamesInUse.test(StationName::townDocks))
                    return StringManager::toTownName(StringIds::station_town_docks);
            }
        }

        // 0x0048FA41
        if (IndustryManager::industryNearPosition(position, IndustryObjectFlags::oilfield))
        {
            if (!realNamesInUse.test(StationName::townOilfield))
                return StringManager::toTownName(StringIds::station_town_oilfield);
        }

        if (IndustryManager::industryNearPosition(position, IndustryObjectFlags::mines))
        {
            if (!realNamesInUse.test(StationName::townMines))
                return StringManager::toTownName(StringIds::station_town_mines);
        }

        // 0x0048FA91
        auto numSurroundingWaterTiles = TileManager::countSurroundingWaterTiles(position);
        if (numSurroundingWaterTiles >= 24)
        {
            auto tile = TileManager::get(position);
            auto* surface = tile.surface();
            if (surface != nullptr && surface->water() == 0)
            {
                // Lakeside
                if (!realNamesInUse.test(StationName::townLakeside))
                    return StringManager::toTownName(StringIds::station_town_lakeside);
            }
        }

        // 0x0048FAEB
        auto numSurroundingTrees = TileManager::countSurroundingTrees(position);
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
            auto* town = TownManager::get(townId);
            auto tile = TileManager::get(Map::Pos2(town->x, town->y));
            auto* surface = tile.surface();
            if (surface != nullptr)
            {
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

        for (auto [name, stringId] : additionalNamePairs)
        {
            if (!realNamesInUse.test(name))
                return StringManager::toTownName(stringId);
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

        for (auto [name, stringId] : ordinalNamePairs)
        {
            if (!ordinalNamesInUse.test(name - StationName::townOrd1))
                return StringManager::toTownName(stringId);
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
        for (auto& station : rawStations())
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

    // 0x0042F2FE
    uint16_t deliverCargoToNearbyStations(const uint8_t cargoType, const uint8_t cargoQty, const Map::Pos2& pos, const Map::TilePos2& size)
    {
        const auto initialLoc = TilePos2(pos) - TilePos2(4, 4);
        const auto catchmentSize = size + TilePos2(8, 8);
        // TODO: Use a fixed size array (max size 15)
        std::vector<std::pair<StationId, uint8_t>> foundStations;
        for (TilePos2 searchOffset{ 0, 0 }; searchOffset.y < catchmentSize.y; ++searchOffset.y)
        {
            for (; searchOffset.x < catchmentSize.x; ++searchOffset.x)
            {
                const auto searchLoc = initialLoc + searchOffset;
                if (!Map::validCoords(searchLoc))
                {
                    continue;
                }

                const auto tile = TileManager::get(searchLoc);
                for (const auto& el : tile)
                {
                    auto* elStation = el.as<StationElement>();
                    if (elStation == nullptr)
                    {
                        continue;
                    }

                    if (elStation->isFlag5() || elStation->isGhost())
                    {
                        continue;
                    }

                    if (foundStations.size() > 15)
                    {
                        break;
                    }
                    auto res = std::find_if(foundStations.begin(), foundStations.end(), [stationId = elStation->stationId()](const std::pair<StationId, uint8_t>& item) { return item.first == stationId; });
                    if (res != foundStations.end())
                    {
                        continue;
                    }
                    auto* station = get(elStation->stationId());
                    if (station == nullptr)
                    {
                        continue;
                    }
                    if (!(station->cargoStats[cargoType].flags & (1 << 1)))
                    {
                        continue;
                    }

                    foundStations.push_back(std::make_pair(elStation->stationId(), station->cargoStats[cargoType].rating));
                }
            }
            searchOffset.x = 0;
        }

        if (foundStations.empty())
        {
            return 0;
        }

        const auto ratingTotal = std::accumulate(foundStations.begin(), foundStations.end(), 0, [](const int32_t a, const std::pair<StationId, uint8_t>& b) { return a + b.second * b.second; });
        if (ratingTotal == 0)
        {
            return 0;
        }

        uint16_t cargoQtyDelivered = 0;
        for (const auto& [stationId, rating] : foundStations)
        {
            auto* station = get(stationId);
            if (station == nullptr)
            {
                continue;
            }

            const auto defaultShare = (rating * rating * cargoQty) / ratingTotal;
            const auto alternateShare = (rating * cargoQty) / 256;
            auto share = std::min(defaultShare, alternateShare);
            if (rating > 66)
            {
                share++;
            }
            cargoQtyDelivered += share;
            station->deliverCargoToStation(cargoType, share);
        }

        return std::min<uint16_t>(cargoQtyDelivered, cargoQty);
    }

    void registerHooks()
    {
        // Can be removed once the createStation function has been implemented (used by place.*Station game commands)
        registerHook(
            0x048F988,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto stationId = (reinterpret_cast<Station*>(regs.esi))->id();
                regs.bx = generateNewStationName(stationId, TownId(regs.ebx), Map::Pos3(regs.ax, regs.cx, regs.dh), regs.dl);
                return 0;
            });
    }
}

OpenLoco::StationId OpenLoco::Station::id() const
{
    // TODO check if this is stored in station structure
    //      otherwise add it when possible
    auto index = (size_t)(this - &StationManager::rawStations()[0]);
    if (index >= Limits::kMaxStations)
    {
        return StationId::null;
    }
    return StationId(index);
}
