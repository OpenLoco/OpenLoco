#include "Station.h"
#include "CompanyManager.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Math/Bound.hpp"
#include "MessageManager.h"
#include "Objects/AirportObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadStationObject.h"
#include "OpenLoco.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include <algorithm>
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Literals;

namespace OpenLoco
{
    constexpr uint8_t min_cargo_rating = 0;
    constexpr uint8_t max_cargo_rating = 200;
    constexpr uint8_t catchmentSize = 4;

    struct CargoSearchState
    {
    private:
        inline static loco_global<uint8_t[map_size], 0x00F00484> _map;
        inline static loco_global<uint32_t, 0x0112C68C> _filter;
        inline static loco_global<uint32_t[max_cargo_stats], 0x0112C690> _score;
        inline static loco_global<uint32_t, 0x0112C710> _producedCargoTypes;
        inline static loco_global<IndustryId_t[max_cargo_stats], 0x0112C7D2> _industry;
        inline static loco_global<uint8_t, 0x0112C7F2> _byte_112C7F2;

    public:
        bool mapHas2(const tile_coord_t x, const tile_coord_t y) const
        {
            return (_map[y * map_columns + x] & (1 << 1)) != 0;
        }

        void mapRemove2(const tile_coord_t x, const tile_coord_t y)
        {
            _map[y * map_columns + x] &= ~(1 << 1);
        }

        void setTile(const tile_coord_t x, const tile_coord_t y, const uint8_t flag)
        {
            _map[y * map_columns + x] |= (1 << flag);
        }

        void resetTile(const tile_coord_t x, const tile_coord_t y, const uint8_t flag)
        {
            _map[y * map_columns + x] &= ~(1 << flag);
        }

        void setTileRegion(tile_coord_t x, tile_coord_t y, int16_t xTileCount, int16_t yTileCount, const uint8_t flag)
        {
            auto xStart = x;
            auto xTileStartCount = xTileCount;
            while (yTileCount > 0)
            {
                while (xTileCount > 0)
                {
                    setTile(x, y, flag);
                    x++;
                    xTileCount--;
                }

                x = xStart;
                xTileCount = xTileStartCount;
                y++;
                yTileCount--;
            }
        }

        void resetTileRegion(tile_coord_t x, tile_coord_t y, int16_t xTileCount, int16_t yTileCount, const uint8_t flag)
        {
            auto xStart = x;
            auto xTileStartCount = xTileCount;
            while (yTileCount > 0)
            {
                while (xTileCount > 0)
                {
                    resetTile(x, y, flag);
                    x++;
                    xTileCount--;
                }

                x = xStart;
                xTileCount = xTileStartCount;
                y++;
                yTileCount--;
            }
        }

        uint32_t filter() const
        {
            return _filter;
        }

        void filter(const uint32_t value)
        {
            _filter = value;
        }

        void resetScores()
        {
            std::fill_n(_score.get(), max_cargo_stats, 0);
        }

        uint32_t score(const uint8_t cargo)
        {
            return _score[cargo];
        }

        void addScore(const uint8_t cargo, const int32_t value)
        {
            _score[cargo] += value;
        }

        uint32_t producedCargoTypes() const
        {
            return _producedCargoTypes;
        }

        void resetProducedCargoTypes()
        {
            _producedCargoTypes = 0;
        }

        void addProducedCargoType(const uint8_t cargoId)
        {
            _producedCargoTypes = _producedCargoTypes | (1 << cargoId);
        }

        void byte_112C7F2(const uint8_t value)
        {
            _byte_112C7F2 = value;
        }

        void resetIndustryMap()
        {
            std::fill_n(_industry.get(), max_cargo_stats, IndustryId::null);
        }

        IndustryId_t getIndustry(const uint8_t cargo) const
        {
            return _industry[cargo];
        }

        void setIndustry(const uint8_t cargo, const IndustryId_t id)
        {
            _industry[cargo] = id;
        }
    };

    static void sub_491BF5(const Pos2& pos, const uint8_t flag);
    static StationElement* getStationElement(const Pos3& pos);

    StationId_t Station::id() const
    {
        // TODO check if this is stored in station structure
        //      otherwise add it when possible
        static loco_global<Station[1024], 0x005E6EDC> _stations;
        auto index = (size_t)(this - _stations);
        if (index > 1024)
        {
            index = StationId::null;
        }
        return (StationId_t)index;
    }

    // 0x0048B23E
    void Station::update()
    {
        updateCargoAcceptance();
    }

    // 0x00492640
    void Station::updateCargoAcceptance()
    {
        CargoSearchState cargoSearchState;
        uint32_t currentAcceptedCargo = calcAcceptedCargo(cargoSearchState);
        uint32_t originallyAcceptedCargo = 0;
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            auto& cs = cargo_stats[cargoId];
            cs.industry_id = cargoSearchState.getIndustry(cargoId);
            if (cs.isAccepted())
            {
                originallyAcceptedCargo |= (1 << cargoId);
            }

            bool isNowAccepted = (currentAcceptedCargo & (1 << cargoId)) != 0;
            cs.isAccepted(isNowAccepted);
        }

        if (originallyAcceptedCargo != currentAcceptedCargo)
        {
            if (owner == CompanyManager::getControllingId())
            {
                alertCargoAcceptanceChange(originallyAcceptedCargo, currentAcceptedCargo);
            }
            invalidateWindow();
        }
    }

    // 0x00492683
    void Station::alertCargoAcceptanceChange(uint32_t oldCargoAcc, uint32_t newCargoAcc)
    {
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            bool acceptedBefore = (oldCargoAcc & (1 << cargoId)) != 0;
            bool acceptedNow = (newCargoAcc & (1 << cargoId)) != 0;
            if (acceptedBefore && !acceptedNow)
            {
                MessageManager::post(
                    MessageType::cargoNoLongerAccepted,
                    owner,
                    id(),
                    cargoId);
            }
            else if (!acceptedBefore && acceptedNow)
            {
                MessageManager::post(
                    MessageType::cargoNowAccepted,
                    owner,
                    id(),
                    cargoId);
            }
        }
    }

    // 0x00491FE0
    // WARNING: this may be called with station (ebp) = -1
    // filter only used if location.x != -1
    uint32_t Station::calcAcceptedCargo(CargoSearchState& cargoSearchState, const Pos2& location, const uint32_t filter)
    {
        cargoSearchState.byte_112C7F2(1);
        cargoSearchState.filter(0);

        if (location.x != -1)
        {
            cargoSearchState.filter(filter);
        }

        cargoSearchState.resetIndustryMap();

        setCatchmentDisplay(1);

        if (location.x != -1)
        {
            sub_491BF5(location, 1);
        }

        cargoSearchState.resetScores();
        cargoSearchState.resetProducedCargoTypes();

        if (this != (Station*)0xFFFFFFFF)
        {
            for (uint16_t i = 0; i < stationTileSize; i++)
            {
                auto pos = stationTiles[i];
                auto stationElement = getStationElement(pos);

                if (stationElement == nullptr)
                {
                    continue;
                }

                cargoSearchState.byte_112C7F2(0);

                if (stationElement->stationType() == StationType::roadStation)
                {
                    auto obj = ObjectManager::get<RoadStationObject>(stationElement->objectId());

                    if (obj->flags & RoadStationFlags::passenger)
                    {
                        cargoSearchState.filter(cargoSearchState.filter() | (1 << obj->var_2C));
                    }
                    else if (obj->flags & RoadStationFlags::freight)
                    {
                        cargoSearchState.filter(cargoSearchState.filter() | ~(1 << obj->var_2C));
                    }
                }
                else
                {
                    cargoSearchState.filter(~0);
                }
            }
        }

        if (cargoSearchState.filter() == 0)
        {
            cargoSearchState.filter(~0);
        }

        for (tile_coord_t ty = 0; ty < map_columns; ty++)
        {
            for (tile_coord_t tx = 0; tx < map_rows; tx++)
            {
                if (cargoSearchState.mapHas2(tx, ty))
                {
                    auto pos = Pos2(tx * tile_size, ty * tile_size);
                    auto tile = TileManager::get(pos);

                    for (auto& el : tile)
                    {
                        if (el.isGhost())
                        {
                            continue;
                        }
                        switch (el.type())
                        {
                            case ElementType::industry:
                            {
                                auto industryEl = el.asIndustry();
                                auto industry = industryEl->industry();

                                if (industry == nullptr || industry->under_construction != 0xFF)
                                {
                                    break;
                                }
                                auto obj = industry->object();

                                if (obj == nullptr)
                                {
                                    break;
                                }

                                for (auto cargoId : obj->required_cargo_type)
                                {
                                    if (cargoId != 0xFF && (cargoSearchState.filter() & (1 << cargoId)))
                                    {
                                        cargoSearchState.addScore(cargoId, 8);
                                        cargoSearchState.setIndustry(cargoId, industry->id());
                                    }
                                }

                                for (auto cargoId : obj->produced_cargo_type)
                                {
                                    if (cargoId != 0xFF && (cargoSearchState.filter() & (1 << cargoId)))
                                    {
                                        cargoSearchState.addProducedCargoType(cargoId);
                                    }
                                }

                                break;
                            }
                            case ElementType::building:
                            {
                                auto buildingEl = el.asBuilding();

                                if (buildingEl == nullptr || buildingEl->has_40() || !buildingEl->hasStationElement())
                                {
                                    break;
                                }

                                auto obj = buildingEl->object();

                                if (obj == nullptr)
                                {
                                    break;
                                }
                                for (int i = 0; i < 2; i++)
                                {
                                    const auto cargoId = obj->producedCargoType[i];
                                    if (cargoId != 0xFF && (cargoSearchState.filter() & (1 << cargoId)))
                                    {
                                        cargoSearchState.addScore(cargoId, obj->var_A6[i]);

                                        if (obj->var_A0[i] != 0)
                                        {
                                            cargoSearchState.addProducedCargoType(cargoId);
                                        }
                                    }
                                }

                                for (int i = 0; i < 2; i++)
                                {
                                    if (obj->var_A4[i] != 0xFF && (cargoSearchState.filter() & (1 << obj->var_A4[i])))
                                    {
                                        cargoSearchState.addScore(obj->var_A4[i], obj->var_A8[i]);
                                    }
                                }

                                // Multi tile buildings should only be counted once so remove the other tiles from the search
                                if (obj->flags & BuildingObjectFlags::large_tile)
                                {
                                    auto index = buildingEl->multiTileIndex();
                                    tile_coord_t xPos = (pos.x - Map::offsets[index].x) / tile_size;
                                    tile_coord_t yPos = (pos.y - Map::offsets[index].y) / tile_size;

                                    cargoSearchState.mapRemove2(xPos + 0, yPos + 0);
                                    cargoSearchState.mapRemove2(xPos + 0, yPos + 1);
                                    cargoSearchState.mapRemove2(xPos + 1, yPos + 0);
                                    cargoSearchState.mapRemove2(xPos + 1, yPos + 1);
                                }

                                break;
                            }
                            default:
                                continue;
                        }
                    }
                }
            }
        }

        uint32_t acceptedCargos = 0;

        for (uint8_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            if (cargoSearchState.score(cargoId) >= 8)
            {
                acceptedCargos |= (1 << cargoId);
            }
        }

        return acceptedCargos;
    }

    static void setStationCatchmentRegion(CargoSearchState& cargoSearchState, TilePos2 minPos, TilePos2 maxPos, const uint8_t flags);

    // 0x00491D70
    // catchment flag should not be shifted (1, 2, 3, 4) and NOT (1 << 0, 1 << 1)
    void Station::setCatchmentDisplay(const uint8_t catchmentFlag)
    {
        CargoSearchState cargoSearchState;
        cargoSearchState.resetTileRegion(0, 0, map_columns, map_rows, catchmentFlag);

        if (this == (Station*)0xFFFFFFFF)
            return;

        if (stationTileSize == 0)
            return;

        for (uint16_t i = 0; i < stationTileSize; i++)
        {
            auto pos = stationTiles[i];
            pos.z &= ~((1 << 1) | (1 << 0));

            auto stationElement = getStationElement(pos);

            if (stationElement == nullptr)
                continue;

            switch (stationElement->stationType())
            {
                case StationType::airport:
                {
                    auto airportObject = ObjectManager::get<AirportObject>(stationElement->objectId());

                    Pos2 minPos(airportObject->min_x * 32, airportObject->min_y * 32);
                    Pos2 maxPos(airportObject->max_x * 32, airportObject->max_y * 32);

                    minPos = Math::Vector::rotate(minPos, stationElement->rotation());
                    maxPos = Math::Vector::rotate(maxPos, stationElement->rotation());

                    minPos.x += pos.x;
                    minPos.y += pos.y;
                    maxPos.x += pos.x;
                    maxPos.y += pos.y;

                    if (minPos.x > maxPos.x)
                    {
                        std::swap(minPos.x, maxPos.x);
                    }

                    if (minPos.y > maxPos.y)
                    {
                        std::swap(minPos.y, maxPos.y);
                    }

                    TilePos2 tileMinPos(minPos);
                    TilePos2 tileMaxPos(maxPos);

                    tileMinPos.x -= catchmentSize;
                    tileMinPos.y -= catchmentSize;
                    tileMaxPos.x += catchmentSize;
                    tileMaxPos.y += catchmentSize;

                    setStationCatchmentRegion(cargoSearchState, tileMinPos, tileMaxPos, catchmentFlag);
                }
                break;
                case StationType::docks:
                {
                    TilePos2 minPos(pos);
                    auto maxPos = minPos;

                    minPos.x -= catchmentSize;
                    minPos.y -= catchmentSize;
                    // Docks are always size 2x2
                    maxPos.x += catchmentSize + 1;
                    maxPos.y += catchmentSize + 1;

                    setStationCatchmentRegion(cargoSearchState, minPos, maxPos, catchmentFlag);
                }
                break;
                default:
                {
                    TilePos2 minPos(pos);
                    auto maxPos = minPos;

                    minPos.x -= catchmentSize;
                    minPos.y -= catchmentSize;
                    maxPos.x += catchmentSize;
                    maxPos.y += catchmentSize;

                    setStationCatchmentRegion(cargoSearchState, minPos, maxPos, catchmentFlag);
                }
            }
        }
    }

    // 0x0049B4E0
    void Station::deliverCargoToTown(uint8_t cargoType, uint16_t cargoQuantity)
    {
        auto twn = TownManager::get(town);
        twn->monthly_cargo_delivered[cargoType] = Math::Bound::add(twn->monthly_cargo_delivered[cargoType], cargoQuantity);
    }

    // 0x0048F7D1
    void Station::sub_48F7D1()
    {
        registers regs;
        regs.ebx = id();
        call(0x0048F7D1, regs);
    }

    // 0x00492A98
    char* Station::getStatusString(char* buffer)
    {
        char* ptr = buffer;
        *ptr = '\0';

        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            auto& stats = cargo_stats[cargoId];

            if (stats.quantity == 0)
                continue;

            if (*buffer != '\0')
                ptr = StringManager::formatString(ptr, StringIds::waiting_cargo_separator);

            loco_global<uint32_t, 0x112C826> _common_format_args;
            *_common_format_args = stats.quantity;

            auto cargo = ObjectManager::get<CargoObject>(cargoId);
            string_id unit_name = stats.quantity == 1 ? cargo->unit_name_singular : cargo->unit_name_plural;
            ptr = StringManager::formatString(ptr, unit_name, &*_common_format_args);
        }

        string_id suffix = *buffer == '\0' ? StringIds::nothing_waiting : StringIds::waiting;
        return StringManager::formatString(ptr, suffix);
    }

    // 0x00492793
    bool Station::updateCargo()
    {
        bool atLeastOneGoodRating = false;
        bool quantityUpdated = false;

        var_3B0 = std::min(var_3B0 + 1, 255);
        var_3B1 = std::min(var_3B1 + 1, 255);

        auto& rng = gPrng();
        for (uint32_t i = 0; i < max_cargo_stats; i++)
        {
            auto& cargo = cargo_stats[i];
            if (!cargo.empty())
            {
                if (cargo.quantity != 0 && cargo.origin != id())
                {
                    cargo.enroute_age = std::min(cargo.enroute_age + 1, 255);
                }
                cargo.age = std::min(cargo.age + 1, 255);

                auto targetRating = calculateCargoRating(cargo);
                // Limit to +/- 2 minimum change
                auto ratingDelta = std::clamp(targetRating - cargo.rating, -2, 2);
                cargo.rating += ratingDelta;

                if (cargo.rating <= 50)
                {
                    // Rating < 25%, decrease cargo
                    if (cargo.quantity >= 400)
                    {
                        cargo.quantity -= rng.randNext(1, 32);
                        quantityUpdated = true;
                    }
                    else if (cargo.quantity >= 200)
                    {
                        cargo.quantity -= rng.randNext(1, 8);
                        quantityUpdated = true;
                    }
                }
                if (cargo.rating >= 100)
                {
                    atLeastOneGoodRating = true;
                }
                if (cargo.rating <= 100 && cargo.quantity != 0)
                {
                    if (cargo.rating <= rng.randNext(0, 127))
                    {
                        cargo.quantity = std::max(0, cargo.quantity - rng.randNext(1, 4));
                        quantityUpdated = true;
                    }
                }
            }
        }

        updateCargoDistribution();

        auto w = WindowManager::find(WindowType::station, id());
        if (w != nullptr && (w->current_tab == 2 || w->current_tab == 1 || quantityUpdated))
        {
            w->invalidate();
        }

        return atLeastOneGoodRating;
    }

    // 0x004927F6
    int32_t Station::calculateCargoRating(const StationCargoStats& cargo) const
    {
        int32_t rating = 0;

        // Bonus if cargo is fresh
        if (cargo.age <= 45)
        {
            rating += 40;
            if (cargo.age <= 30)
            {
                rating += 45;
                if (cargo.age <= 15)
                {
                    rating += 45;
                    if (cargo.age <= 7)
                    {
                        rating += 35;
                    }
                }
            }
        }

        // Penalty if lots of cargo waiting
        rating -= 130;
        if (cargo.quantity <= 1000)
        {
            rating += 30;
            if (cargo.quantity <= 500)
            {
                rating += 30;
                if (cargo.quantity <= 300)
                {
                    rating += 30;
                    if (cargo.quantity <= 200)
                    {
                        rating += 20;
                        if (cargo.quantity <= 100)
                        {
                            rating += 20;
                        }
                    }
                }
            }
        }

        if ((flags & (StationFlags::flag_7 | StationFlags::flag_8)) == 0 && !isPlayerCompany(owner))
        {
            rating = 120;
        }

        Speed16 vehicleSpeed = std::min(cargo.vehicleSpeed, 250_mph);
        if (vehicleSpeed > 35_mph)
        {
            rating += ((vehicleSpeed - 35_mph).getRaw()) / 4;
        }

        if (cargo.vehicleAge < 4)
        {
            rating += 10;
            if (cargo.vehicleAge < 2)
            {
                rating += 10;
                if (cargo.vehicleAge < 1)
                {
                    rating += 13;
                }
            }
        }

        return std::clamp<int32_t>(rating, min_cargo_rating, max_cargo_rating);
    }

    // 0x004929DB
    void Station::updateCargoDistribution()
    {
        WindowManager::invalidate(Ui::WindowType::station, id());
        WindowManager::invalidate(Ui::WindowType::stationList);
        bool hasChanged = false;
        for (uint8_t i = 0; i < max_cargo_stats; ++i)
        {
            auto& cargoStat = cargo_stats[i];
            auto newAmount = 0;
            if (cargoStat.quantity != 0)
            {
                if (stationTileSize != 0)
                {
                    newAmount = cargoStat.quantity / stationTileSize;
                    auto* cargoObj = ObjectManager::get<CargoObject>(i);
                    newAmount += (1 << cargoObj->var_14) - 1;
                    newAmount >>= cargoObj->var_14;

                    newAmount = std::min(newAmount, 15);
                }
            }
            if (cargoStat.var_40 != newAmount)
            {
                cargoStat.var_40 = newAmount;
                hasChanged = true;
            }
        }

        if (hasChanged)
        {
            for (auto i = 0; i < stationTileSize; ++i)
            {
                const auto& tile = stationTiles[i];
                Ui::ViewportManager::invalidate({ tile.x, tile.y }, tile.z & 0xFFFC, tile.z + 32, ZoomLevel::full);
            }
        }
    }

    // 0x0048DCA5
    void Station::updateLabel()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x0048DCA5, regs);
    }

    // 0x004CBA2D
    void Station::invalidate()
    {
        Ui::ViewportManager::invalidate(this);
    }

    void Station::invalidateWindow()
    {
        WindowManager::invalidate(WindowType::station, id());
    }

    // 0x0048F6D4
    static StationElement* getStationElement(const Pos3& pos)
    {
        auto tile = TileManager::get(pos.x, pos.y);
        auto baseZ = pos.z / 4;

        for (auto& element : tile)
        {
            auto stationElement = element.asStation();

            if (stationElement == nullptr)
            {
                continue;
            }

            if (stationElement->baseZ() != baseZ)
            {
                continue;
            }

            if (!stationElement->isFlag5())
            {
                return stationElement;
            }
            else
            {
                return nullptr;
            }
        }
        return nullptr;
    }

    // 0x00491EDC
    static void setStationCatchmentRegion(CargoSearchState& cargoSearchState, TilePos2 minPos, TilePos2 maxPos, const uint8_t flag)
    {
        minPos.x = std::max(minPos.x, static_cast<coord_t>(0));
        minPos.y = std::max(minPos.y, static_cast<coord_t>(0));
        maxPos.x = std::min(maxPos.x, static_cast<coord_t>(map_columns - 1));
        maxPos.y = std::min(maxPos.y, static_cast<coord_t>(map_rows - 1));

        maxPos.x -= minPos.x;
        maxPos.y -= minPos.y;
        maxPos.x++;
        maxPos.y++;

        cargoSearchState.setTileRegion(minPos.x, minPos.y, maxPos.x, maxPos.y, flag);
    }

    // 0x00491BF5
    static void sub_491BF5(const Pos2& pos, const uint8_t flag)
    {
        TilePos2 minPos(pos);
        auto maxPos = minPos;
        maxPos.x += catchmentSize;
        maxPos.y += catchmentSize;
        minPos.x -= catchmentSize;
        minPos.y -= catchmentSize;

        CargoSearchState cargoSearchState;

        setStationCatchmentRegion(cargoSearchState, minPos, maxPos, flag);
    }

    string_id getTransportIconsFromStationFlags(const uint16_t flags)
    {
        constexpr string_id label_icons[] = {
            StringIds::label_icons_none,
            StringIds::label_icons_rail,
            StringIds::label_icons_road,
            StringIds::label_icons_rail_road,
            StringIds::label_icons_air,
            StringIds::label_icons_rail_air,
            StringIds::label_icons_road_air,
            StringIds::label_icons_rail_road_air,
            StringIds::label_icons_water,
            StringIds::label_icons_rail_water,
            StringIds::label_icons_road_water,
            StringIds::label_icons_rail_road_water,
            StringIds::label_icons_air_water,
            StringIds::label_icons_rail_air_water,
            StringIds::label_icons_road_air_water,
            StringIds::label_icons_rail_road_air_water,
        };
        return label_icons[flags & StationFlags::allModes];
    }
}
