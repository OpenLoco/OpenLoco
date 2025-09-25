#include "StationManager.h"
#include "CompanyManager.h"
#include "Config.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "IndustryManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "MessageManager.h"
#include "Objects/IndustryObject.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "TownManager.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/VehicleManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Vector.hpp>

#include <bitset>
#include <numeric>
#include <sfl/static_vector.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::World;

namespace OpenLoco::StationManager
{
    constexpr auto kStationDistanceLimit = 8 * World::kTileSize;

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
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !SceneManager::isEditorMode())
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
                station.noTilesTimeout++;
                if (station.noTilesTimeout == 5 && CompanyManager::isPlayerCompany(station.owner))
                {
                    companyEmotionEvent(station.owner, Emotion::disgusted);
                }
                if (station.noTilesTimeout >= 10)
                {
                    sub_49E1F1(station.id());
                    station.invalidate();
                    deallocateStation(station.id());
                }
            }
            else
            {
                station.noTilesTimeout = 0;
            }
            if (station.updateCargo())
            {
                auto town = TownManager::get(station.town);
                if (town != nullptr && (town->flags & TownFlags::ratingAdjusted) == TownFlags::none)
                {
                    town->flags |= TownFlags::ratingAdjusted;
                    town->adjustCompanyRating(station.owner, 1);
                }
            }
        }
    }

    // 0x048F988
    StringId generateNewStationName(StationId stationId, TownId townId, World::Pos3 position, uint8_t mode)
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
            {
                continue;
            }

            auto nameKey = StringManager::fromTownName(station.name) - StringIds::station_town;
            if (nameKey > StationName::townOrd20)
            {
                continue;
            }

            if (station.town != townId)
            {
                continue;
            }

            if (nameKey < StationName::townOrd1)
            {
                realNamesInUse.set(nameKey, true);
            }
            else
            {
                ordinalNamesInUse.set(nameKey - StationName::townOrd1, true);
            }
        }

        if (mode == 1)
        {
            // Airport
            if (!realNamesInUse.test(StationName::townAirport))
            {
                return StringManager::toTownName(StringIds::station_town_airport);
            }
        }
        else if (mode == 2)
        {
            // Heliport
            if (!realNamesInUse.test(StationName::townHeliport))
            {
                return StringManager::toTownName(StringIds::station_town_heliport);
            }
        }
        else if (mode == 3)
        {
            // 0x0048FA00
            auto tile = TileManager::get(World::Pos2(position.x, position.y));
            auto* surface = tile.surface();
            if (surface != nullptr && surface->water() == 0)
            {
                // Docks
                if (!realNamesInUse.test(StationName::townDocks))
                {
                    return StringManager::toTownName(StringIds::station_town_docks);
                }
            }
        }

        // 0x0048FA41
        if (IndustryManager::industryNearPosition(position, IndustryObjectFlags::oilfieldStationName))
        {
            if (!realNamesInUse.test(StationName::townOilfield))
            {
                return StringManager::toTownName(StringIds::station_town_oilfield);
            }
        }

        if (IndustryManager::industryNearPosition(position, IndustryObjectFlags::minesStationName))
        {
            if (!realNamesInUse.test(StationName::townMines))
            {
                return StringManager::toTownName(StringIds::station_town_mines);
            }
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
                {
                    return StringManager::toTownName(StringIds::station_town_lakeside);
                }
            }
        }

        // 0x0048FAEB
        auto numSurroundingTrees = TileManager::countSurroundingTrees(position);
        if (numSurroundingTrees > 40)
        {
            // Forest
            if (!realNamesInUse.test(StationName::townForest))
            {
                return StringManager::toTownName(StringIds::station_town_forest);
            }
        }
        else if (numSurroundingTrees > 20)
        {
            // Woods
            if (!realNamesInUse.test(StationName::townWoods))
            {
                return StringManager::toTownName(StringIds::station_town_woods);
            }
        }

        // 0x0048FB29
        {
            auto* town = TownManager::get(townId);
            auto tile = TileManager::get(World::Pos2(town->x, town->y));
            auto* surface = tile.surface();
            if (surface != nullptr)
            {
                auto townHeightDiff = (position.z / 4) - surface->baseZ();
                if (townHeightDiff > 20)
                {
                    // Heights
                    if (!realNamesInUse.test(StationName::townHeights))
                    {
                        return StringManager::toTownName(StringIds::station_town_heights);
                    }
                }
                else if (townHeightDiff < -20)
                {
                    // Valley
                    if (!realNamesInUse.test(StationName::townValley))
                    {
                        return StringManager::toTownName(StringIds::station_town_valley);
                    }
                }
            }
        }

        // 0x0048FB8B
        if (!realNamesInUse.test(StationName::townDefault))
        {
            return StringManager::toTownName(StringIds::station_town);
        }

        auto town = TownManager::get(townId);
        {
            auto manhattanDistance = Math::Vector::manhattanDistance2D(position, World::Pos2{ town->x, town->y });
            if (manhattanDistance / World::kTileSize <= 9)
            {
                // Central
                if (!realNamesInUse.test(StationName::townCentral))
                {
                    return StringManager::toTownName(StringIds::station_town_central);
                }
            }
        }

        if (position.x <= town->x && position.y <= town->y)
        {
            // North
            if (!realNamesInUse.test(StationName::townNorth))
            {
                return StringManager::toTownName(StringIds::station_town_north);
            }
        }

        if (position.x >= town->x && position.y >= town->y)
        {
            // South
            if (!realNamesInUse.test(StationName::townSouth))
            {
                return StringManager::toTownName(StringIds::station_town_south);
            }
        }

        if (position.x <= town->x && position.y >= town->y)
        {
            // East
            if (!realNamesInUse.test(StationName::townEast))
            {
                return StringManager::toTownName(StringIds::station_town_east);
            }
        }

        if (position.x >= town->x && position.y <= town->y)
        {
            // West
            if (!realNamesInUse.test(StationName::townWest))
            {
                return StringManager::toTownName(StringIds::station_town_west);
            }
        }

        // Additional names to try
        static constexpr std::pair<const StationName, const StringId> additionalNamePairs[] = {
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
            {
                return StringManager::toTownName(stringId);
            }
        }

        // Ordinal names to try
        static constexpr std::pair<const StationName, const StringId> ordinalNamePairs[] = {
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
            {
                return StringManager::toTownName(stringId);
            }
        }

        // Default to an ordinal string instead, e.g. 'Station 42'.
        char stationName[256] = "";
        FormatArguments args{};
        args.push(stationId);
        StringManager::formatString(stationName, StringIds::station_name_ordinal, args);
        return StringManager::userStringAllocate(stationName, true);
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

    using CargoStations = sfl::static_vector<std::pair<StationId, uint8_t>, 16>;

    static CargoStations findStationsForCargoType(const uint8_t cargoType, const World::Pos2& pos, const World::TilePos2& size)
    {
        const auto initialLoc = World::toTileSpace(pos) - TilePos2(4, 4);
        const auto catchmentSize = size + TilePos2(8, 8);

        CargoStations foundStations;
        for (TilePos2 searchOffset{ 0, 0 }; searchOffset.y < catchmentSize.y; ++searchOffset.y)
        {
            for (; searchOffset.x < catchmentSize.x; ++searchOffset.x)
            {
                const auto searchLoc = initialLoc + searchOffset;
                if (!World::TileManager::validCoords(searchLoc))
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

                    if (elStation->isAiAllocated() || elStation->isGhost())
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
                    if ((station->cargoStats[cargoType].flags & StationCargoStatsFlags::acceptedFromProducer) == StationCargoStatsFlags::none)
                    {
                        continue;
                    }

                    foundStations.push_back(std::make_pair(elStation->stationId(), station->cargoStats[cargoType].rating));
                }
            }
            searchOffset.x = 0;
        }

        return foundStations;
    }

    static uint16_t deliverCargoToStations(const CargoStations& foundStations, const uint8_t cargoType, const uint8_t cargoQty)
    {
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

    // 0x0042F2FE
    uint16_t deliverCargoToNearbyStations(const uint8_t cargoType, const uint8_t cargoQty, const World::Pos2& pos, const World::TilePos2& size)
    {
        const auto foundStations = findStationsForCargoType(cargoType, pos, size);
        if (foundStations.empty())
        {
            return 0;
        }

        return deliverCargoToStations(foundStations, cargoType, cargoQty);
    }

    // 0x0042F2BF
    uint16_t deliverCargoToStations(std::span<const StationId> stations, const uint8_t cargoType, const uint8_t cargoQty)
    {
        CargoStations foundStations;
        for (auto stationId : stations)
        {
            auto* station = get(stationId);
            if (station == nullptr)
            {
                continue;
            }
            foundStations.push_back(std::make_pair(stationId, station->cargoStats[cargoType].rating));
        }

        if (foundStations.empty())
        {
            return 0;
        }

        return deliverCargoToStations(foundStations, cargoType, cargoQty);
    }

    // 0x0048FEF4
    bool exceedsStationSize(Station& station, World::Pos3 pos)
    {
        if (Config::get().disableStationSizeLimit)
        {
            return false;
        }

        auto centreTile = World::Pos2(pos.x + World::kTileSize / 2, pos.y + World::kTileSize / 2);
        auto stationCentre = World::Pos2(station.x, station.y);
        auto distance = Math::Vector::chebyshevDistance2D(centreTile, stationCentre);

        return distance > kStationDistanceLimit;
    }

    // 0x0048F8A0
    StationId allocateNewStation(const World::Pos3 pos, const CompanyId owner, const uint8_t mode)
    {
        if (!World::TileManager::validCoords(pos))
        {
            GameCommands::setErrorTitle(StringIds::off_edge_of_map);
            return StationId::null;
        }

        // Find first available station
        for (auto& station : rawStations())
        {
            if (!station.empty())
            {
                continue;
            }

            auto maybeTown = TownManager::getClosestTownAndDensity(pos);
            if (!maybeTown)
            {
                GameCommands::setErrorTitle(StringIds::town_must_be_built_first);
                return StationId::null;
            }

            station.town = maybeTown->first;
            station.owner = owner;
            station.name = generateNewStationName(station.id(), station.town, pos, mode);

            // Reset cargo stats
            for (auto& stats : station.cargoStats)
            {
                stats.quantity = 0;
                stats.origin = StationId::null;
                stats.flags = StationCargoStatsFlags::none;
                stats.rating = 150;
                stats.densityPerTile = 0;
            }

            station.x = pos.x;
            station.y = pos.y;
            station.z = pos.z;
            station.flags = StationFlags::flag_5;
            station.stationTileSize = 0;
            station.noTilesTimeout = 0;
            station.var_3B0 = 0;
            station.var_3B1 = 0;

            return station.id();
        }

        GameCommands::setErrorTitle(StringIds::too_many_stations_in_game);
        return StationId::null;
    }

    // 0x0048F850
    static void removeStationFromCargoStats(const StationId removedStationId)
    {
        for (auto& station : stations())
        {
            for (auto& stats : station.cargoStats)
            {
                if (stats.origin == removedStationId)
                {
                    stats.origin = StationId::null;
                    stats.quantity = 0;
                    Ui::WindowManager::invalidate(Ui::WindowType::station, enumValue(station.id()));
                }
            }
        }
    }

    // 0x0048F7D1
    void deallocateStation(const StationId stationId)
    {
        WindowManager::close(WindowType::station, WindowNumber_t(stationId));

        auto station = get(stationId);
        if (station == nullptr)
        {
            return;
        }

        if ((station->flags & StationFlags::flag_5) == StationFlags::none)
        {
            auto town = TownManager::get(station->town);
            if (town != nullptr)
            {
                town->numStations--;
                WindowManager::invalidate(WindowType::town, WindowNumber_t(station->town));
            }
        }

        Windows::StationList::removeStationFromList(stationId);
        Vehicles::OrderManager::removeOrdersForStation(stationId);
        removeStationFromCargoStats(stationId);
        VehicleManager::resetIfHeadingForStation(stationId);

        MessageManager::removeAllSubjectRefs(enumValue(stationId), MessageItemArgumentType::station);
        StringManager::emptyUserString(station->name);
        station->name = StringIds::null;
    }

    StationId findNearbyEmptyStation(const World::Pos3 pos, const CompanyId companyId, const int16_t currentMinDistanceStation)
    {
        // After removing a station the station is still available for a time but left empty
        // this function will find these empty stations so that a new station can reuse the
        // empty station and its name.
        auto minDistanceStation = StationId::null;
        auto minDistance = currentMinDistanceStation;
        for (const auto& station : StationManager::stations())
        {
            if (station.stationTileSize != 0)
            {
                continue;
            }
            if (station.owner != companyId)
            {
                continue;
            }
            const auto distance = Math::Vector::chebyshevDistance2D(World::Pos2{ station.x, station.y }, pos);

            auto distDiffZ = std::abs(station.z - pos.z);
            if (distDiffZ > kMaxStationNearbyDistance)
            {
                continue;
            }
            if (distance > kMaxStationNearbyDistance)
            {
                continue;
            }
            if (distance < minDistance)
            {
                minDistance = distance;
                minDistanceStation = station.id();
            }
        }
        return minDistanceStation;
    }

    // 0x004901B0
    NearbyStation findNearbyStation(World::Pos3 pos, CompanyId companyId)
    {
        const auto tilePosA = World::toTileSpace(pos) - World::TilePos2(2, 2);
        const auto tilePosB = World::toTileSpace(pos) + World::TilePos2(2, 2);

        auto minDistanceStation = StationId::null;
        auto minDistance = std::numeric_limits<int16_t>::max();
        bool isPhysicallyAttached = false;
        for (const auto& tilePos : World::getClampedRange(tilePosA, tilePosB))
        {
            const auto tile = World::TileManager::get(tilePos);
            for (const auto& el : tile)
            {
                auto* elStation = el.as<World::StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }
                if (elStation->isGhost())
                {
                    continue;
                }
                auto* station = StationManager::get(elStation->stationId());
                if (station->owner != companyId)
                {
                    continue;
                }

                const auto distance = Math::Vector::chebyshevDistance2D(World::toWorldSpace(tilePos), pos);
                if (distance < minDistance)
                {
                    auto distDiffZ = std::abs(elStation->baseHeight() - pos.z);
                    if (distDiffZ > kMaxStationNearbyDistance)
                    {
                        continue;
                    }
                    minDistance = distance + distDiffZ / 2;
                    if (minDistance <= kMaxStationNearbyDistance)
                    {
                        isPhysicallyAttached = true;
                    }
                    minDistanceStation = elStation->stationId();
                }
            }
        }

        const auto nearbyEmptyStation = findNearbyEmptyStation(pos, companyId, minDistance);
        if (nearbyEmptyStation != StationId::null)
        {
            return NearbyStation{ nearbyEmptyStation, isPhysicallyAttached };
        }
        else
        {
            return NearbyStation{ minDistanceStation, isPhysicallyAttached };
        }
    }

    void registerHooks()
    {
        // Can be removed once the createStation function has been implemented (used by place.*Station game commands)
        registerHook(
            0x048F988,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto stationId = (reinterpret_cast<Station*>(regs.esi))->id();
                const auto newName = generateNewStationName(stationId, TownId(regs.ebx), World::Pos3(regs.ax, regs.cx, regs.dh * World::kSmallZStep), regs.dl);
                regs = backup;
                regs.bx = newName;
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
