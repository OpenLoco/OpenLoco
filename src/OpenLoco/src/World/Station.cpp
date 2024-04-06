#include "Station.h"
#include "CompanyManager.h"
#include "IndustryManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "MessageManager.h"
#include "Objects/AirportObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "StationManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Bound.hpp>
#include <algorithm>
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Literals;

namespace OpenLoco
{
    constexpr uint8_t kMinCargoRating = 0;
    constexpr uint8_t kMaxCargoRating = 200;
    constexpr uint8_t catchmentSize = 4;

    struct CargoSearchState
    {
    private:
        inline static loco_global<uint8_t[kMapSize], 0x00F00484> _map;
        inline static loco_global<uint32_t, 0x0112C68C> _filter;
        inline static loco_global<uint32_t[kMaxCargoStats], 0x0112C690> _score;
        inline static loco_global<uint32_t, 0x0112C710> _producedCargoTypes;
        inline static loco_global<IndustryId[kMaxCargoStats], 0x0112C7D2> _industry;
        inline static loco_global<uint8_t, 0x0112C7F2> _byte_112C7F2;

    public:
        bool mapHas2(const tile_coord_t x, const tile_coord_t y) const
        {
            return (_map[y * kMapColumns + x] & (1 << enumValue(CatchmentFlags::flag_1))) != 0;
        }

        void mapRemove2(const tile_coord_t x, const tile_coord_t y)
        {
            _map[y * kMapColumns + x] &= ~(1 << enumValue(CatchmentFlags::flag_1));
        }

        void setTile(const tile_coord_t x, const tile_coord_t y, const CatchmentFlags flag)
        {
            _map[y * kMapColumns + x] |= (1 << enumValue(flag));
        }

        void resetTile(const tile_coord_t x, const tile_coord_t y, const CatchmentFlags flag)
        {
            _map[y * kMapColumns + x] &= ~(1 << enumValue(flag));
        }

        void setTileRegion(tile_coord_t x, tile_coord_t y, int16_t xTileCount, int16_t yTileCount, const CatchmentFlags flag)
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

        void resetTileRegion(tile_coord_t x, tile_coord_t y, int16_t xTileCount, int16_t yTileCount, const CatchmentFlags flag)
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
            std::fill_n(_score.get(), kMaxCargoStats, 0);
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

        bool cargoFilterHasBeenRecalculated() const { return _byte_112C7F2 == 0; }

        void resetIndustryMap()
        {
            std::fill_n(_industry.get(), kMaxCargoStats, IndustryId::null);
        }

        IndustryId getIndustry(const uint8_t cargo) const
        {
            return _industry[cargo];
        }

        void setIndustry(const uint8_t cargo, const IndustryId id)
        {
            _industry[cargo] = id;
        }
    };

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
        for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            auto& stationCargoStats = cargoStats[cargoId];
            stationCargoStats.industryId = cargoSearchState.getIndustry(cargoId);
            if (stationCargoStats.isAccepted())
            {
                originallyAcceptedCargo |= (1 << cargoId);
            }

            bool isNowAccepted = (currentAcceptedCargo & (1 << cargoId)) != 0;
            stationCargoStats.isAccepted(isNowAccepted);
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
        for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            bool acceptedBefore = (oldCargoAcc & (1 << cargoId)) != 0;
            bool acceptedNow = (newCargoAcc & (1 << cargoId)) != 0;
            if (acceptedBefore && !acceptedNow)
            {
                MessageManager::post(
                    MessageType::cargoNoLongerAccepted,
                    owner,
                    enumValue(id()),
                    cargoId);
            }
            else if (!acceptedBefore && acceptedNow)
            {
                MessageManager::post(
                    MessageType::cargoNowAccepted,
                    owner,
                    enumValue(id()),
                    cargoId);
            }
        }
    }

    // 0x00492026
    static uint32_t doCalcAcceptedCargo(const Station* station, CargoSearchState& cargoSearchState)
    {
        cargoSearchState.resetScores();
        cargoSearchState.resetProducedCargoTypes();

        if (station != nullptr)
        {
            // See also recalculateStationModes which has a similar loop
            for (uint16_t i = 0; i < station->stationTileSize; i++)
            {
                auto pos = station->stationTiles[i];
                auto stationElement = getStationElement(pos);

                if (stationElement == nullptr)
                {
                    continue;
                }

                cargoSearchState.byte_112C7F2(0);

                if (stationElement->stationType() == StationType::roadStation)
                {
                    auto obj = ObjectManager::get<RoadStationObject>(stationElement->objectId());

                    if (obj->hasFlags(RoadStationFlags::passenger))
                    {
                        cargoSearchState.filter(cargoSearchState.filter() | (1 << obj->cargoType));
                    }
                    else if (obj->hasFlags(RoadStationFlags::freight))
                    {
                        cargoSearchState.filter(cargoSearchState.filter() | ~(1 << obj->cargoType));
                    }
                }
                else
                {
                    cargoSearchState.filter(~0U);
                }
            }
        }

        if (cargoSearchState.filter() == 0)
        {
            cargoSearchState.filter(~0U);
        }

        for (tile_coord_t ty = 0; ty < kMapColumns; ty++)
        {
            for (tile_coord_t tx = 0; tx < kMapRows; tx++)
            {
                if (cargoSearchState.mapHas2(tx, ty))
                {
                    auto pos = Pos2(tx * kTileSize, ty * kTileSize);
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
                                auto& industryEl = el.get<IndustryElement>();
                                auto* industry = industryEl.industry();

                                if (industry == nullptr || industry->under_construction != 0xFF)
                                {
                                    break;
                                }
                                const auto* obj = industry->getObject();

                                if (obj == nullptr)
                                {
                                    break;
                                }

                                for (auto cargoId : obj->requiredCargoType)
                                {
                                    if (cargoId != 0xFF && (cargoSearchState.filter() & (1 << cargoId)))
                                    {
                                        cargoSearchState.addScore(cargoId, 8);
                                        cargoSearchState.setIndustry(cargoId, industry->id());
                                    }
                                }

                                for (auto cargoId : obj->producedCargoType)
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
                                auto& buildingEl = el.get<BuildingElement>();

                                if (buildingEl.has_40() || !buildingEl.isConstructed())
                                {
                                    break;
                                }

                                const auto* obj = buildingEl.getObject();

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

                                        if (obj->producedQuantity[i] != 0)
                                        {
                                            cargoSearchState.addProducedCargoType(cargoId);
                                        }
                                    }
                                }

                                for (int i = 0; i < 2; i++)
                                {
                                    if (obj->requiredCargoType[i] != 0xFF && (cargoSearchState.filter() & (1 << obj->requiredCargoType[i])))
                                    {
                                        cargoSearchState.addScore(obj->requiredCargoType[i], obj->var_A8[i]);
                                    }
                                }

                                // Multi tile buildings should only be counted once so remove the other tiles from the search
                                if (obj->hasFlags(BuildingObjectFlags::largeTile))
                                {
                                    auto index = buildingEl.multiTileIndex();
                                    tile_coord_t xPos = (pos.x - World::kOffsets[index].x) / kTileSize;
                                    tile_coord_t yPos = (pos.y - World::kOffsets[index].y) / kTileSize;

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

        for (uint8_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            if (cargoSearchState.score(cargoId) >= 8)
            {
                acceptedCargos |= (1 << cargoId);
            }
        }

        return acceptedCargos;
    }

    // 0x00491FE0
    // THIS FUNCTION ONLY TO BE CALLED ON NORMAL STATIONS
    uint32_t Station::calcAcceptedCargo(CargoSearchState& cargoSearchState) const
    {
        cargoSearchState.byte_112C7F2(1);
        cargoSearchState.filter(0);

        cargoSearchState.resetIndustryMap();

        setCatchmentDisplay(this, CatchmentFlags::flag_1);

        return doCalcAcceptedCargo(this, cargoSearchState);
    }

    // 0x00491FE0
    // WARNING: this may be called with station (ebp) = -1
    // THIS FUNCTION ONLY TO BE CALLED ON GHOST TRACK STATIONS
    PotentialCargo calcAcceptedCargoTrainStationGhost(const Station* ghostStation, const Pos2& location, const uint32_t filter)
    {
        CargoSearchState cargoSearchState;
        cargoSearchState.byte_112C7F2(1);
        cargoSearchState.filter(0);

        cargoSearchState.filter(filter);

        cargoSearchState.resetIndustryMap();

        setCatchmentDisplay(ghostStation, CatchmentFlags::flag_1);

        sub_491BF5(location, CatchmentFlags::flag_1);

        PotentialCargo res{};
        res.accepted = doCalcAcceptedCargo(ghostStation, cargoSearchState);
        res.produced = cargoSearchState.producedCargoTypes();
        return res;
    }

    // 0x00491F43
    // WARNING: this may be called with station (ebp) = -1
    // THIS FUNCTION ONLY TO BE CALLED ON GHOST AIRPORT STATIONS
    PotentialCargo calcAcceptedCargoAirportGhost(const Station* ghostStation, const uint8_t type, const Pos2& location, const uint8_t rotation, const uint32_t filter)
    {
        CargoSearchState cargoSearchState;
        cargoSearchState.byte_112C7F2(1);
        cargoSearchState.filter(0);

        cargoSearchState.filter(filter);

        cargoSearchState.resetIndustryMap();

        setCatchmentDisplay(ghostStation, CatchmentFlags::flag_1);

        sub_491C6F(type, location, rotation, CatchmentFlags::flag_1);

        PotentialCargo res{};
        res.accepted = doCalcAcceptedCargo(ghostStation, cargoSearchState);
        res.produced = cargoSearchState.producedCargoTypes();
        return res;
    }

    // 0x00491F93
    // WARNING: this may be called with station (ebp) = -1
    // THIS FUNCTION ONLY TO BE CALLED ON GHOST DOCK STATIONS
    PotentialCargo calcAcceptedCargoDockGhost(const Station* ghostStation, const Pos2& location, const uint32_t filter)
    {
        CargoSearchState cargoSearchState;
        cargoSearchState.byte_112C7F2(1);
        cargoSearchState.filter(0);

        cargoSearchState.filter(filter);

        cargoSearchState.resetIndustryMap();

        setCatchmentDisplay(ghostStation, CatchmentFlags::flag_1);

        sub_491D20(location, CatchmentFlags::flag_1);

        PotentialCargo res{};
        res.accepted = doCalcAcceptedCargo(ghostStation, cargoSearchState);
        res.produced = cargoSearchState.producedCargoTypes();
        return res;
    }

    static void setStationCatchmentRegion(CargoSearchState& cargoSearchState, TilePos2 minPos, TilePos2 maxPos, const CatchmentFlags flags);

    // 0x00491D70
    // catchment flag should not be shifted (1, 2, 3, 4) and NOT (1 << 0, 1 << 1)
    void setCatchmentDisplay(const Station* station, const CatchmentFlags catchmentFlag)
    {
        CargoSearchState cargoSearchState;
        cargoSearchState.resetTileRegion(0, 0, kMapColumns, kMapRows, catchmentFlag);

        if (station == nullptr)
            return;

        if (station->stationTileSize == 0)
            return;

        for (uint16_t i = 0; i < station->stationTileSize; i++)
        {
            auto pos = station->stationTiles[i];
            pos.z = World::heightFloor(pos.z);

            auto stationElement = getStationElement(pos);

            if (stationElement == nullptr)
                continue;

            switch (stationElement->stationType())
            {
                case StationType::airport:
                {
                    auto airportObject = ObjectManager::get<AirportObject>(stationElement->objectId());

                    auto [minPos, maxPos] = airportObject->getAirportExtents(World::toTileSpace(pos), stationElement->rotation());

                    minPos.x -= catchmentSize;
                    minPos.y -= catchmentSize;
                    maxPos.x += catchmentSize;
                    maxPos.y += catchmentSize;

                    setStationCatchmentRegion(cargoSearchState, minPos, maxPos, catchmentFlag);
                }
                break;
                case StationType::docks:
                {
                    auto minPos = World::toTileSpace(pos);
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
                    auto minPos = World::toTileSpace(pos);
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
        twn->monthlyCargoDelivered[cargoType] = Math::Bound::add(twn->monthlyCargoDelivered[cargoType], cargoQuantity);
    }

    // 0x0042F489
    void Station::deliverCargoToStation(const uint8_t cargoType, const uint8_t cargoQuantity)
    {
        auto& stationCargoStat = cargoStats[cargoType];
        stationCargoStat.quantity = Math::Bound::add(stationCargoStat.quantity, cargoQuantity);
        stationCargoStat.enrouteAge = 0;
        stationCargoStat.origin = id();
        updateCargoDistribution();
    }

    // 0x00492A98
    char* Station::getStatusString(char* buffer)
    {
        char* ptr = buffer;
        *ptr = '\0';

        for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            auto& stationCargoStat = cargoStats[cargoId];

            if (stationCargoStat.quantity == 0)
                continue;

            if (*buffer != '\0')
                ptr = StringManager::formatString(ptr, StringIds::waiting_cargo_separator);

            FormatArguments args{};
            args.push<uint32_t>(stationCargoStat.quantity);

            auto cargo = ObjectManager::get<CargoObject>(cargoId);
            auto unitName = stationCargoStat.quantity == 1 ? cargo->unitNameSingular : cargo->unitNamePlural;
            ptr = StringManager::formatString(ptr, unitName, &args);
        }

        StringId suffix = *buffer == '\0' ? StringIds::nothing_waiting : StringIds::waiting;
        return StringManager::formatString(ptr, suffix);
    }

    // 0x00492793
    bool Station::updateCargo()
    {
        bool atLeastOneGoodRating = false;
        bool quantityUpdated = false;

        var_3B0 = std::min(var_3B0 + 1, 255);
        var_3B1 = std::min(var_3B1 + 1, 255);

        auto& rng = gPrng1();
        for (uint32_t i = 0; i < kMaxCargoStats; i++)
        {
            auto& stationCargo = cargoStats[i];
            if (!stationCargo.empty())
            {
                if (stationCargo.quantity != 0 && stationCargo.origin != id())
                {
                    stationCargo.enrouteAge = std::min(stationCargo.enrouteAge + 1, 255);
                }
                else
                {
                    // Change from vanilla to deal with the cargo transfer bug:
                    // Reset en-route age once the station cargo gets cleared
                    // or else the age keeps increasing
                    stationCargo.enrouteAge = 0;
                }
                stationCargo.age = std::min(stationCargo.age + 1, 255);

                auto targetRating = calculateCargoRating(stationCargo);
                // Limit to +/- 2 minimum change
                auto ratingDelta = std::clamp(targetRating - stationCargo.rating, -2, 2);
                stationCargo.rating += ratingDelta;

                if (stationCargo.rating <= 50)
                {
                    // Rating < 25%, decrease cargo
                    if (stationCargo.quantity >= 400)
                    {
                        stationCargo.quantity -= rng.randNext(1, 32);
                        quantityUpdated = true;
                    }
                    else if (stationCargo.quantity >= 200)
                    {
                        stationCargo.quantity -= rng.randNext(1, 8);
                        quantityUpdated = true;
                    }
                }
                if (stationCargo.rating >= 100)
                {
                    atLeastOneGoodRating = true;
                }
                if (stationCargo.rating <= 100 && stationCargo.quantity != 0)
                {
                    if (stationCargo.rating <= rng.randNext(0, 127))
                    {
                        stationCargo.quantity = std::max(0, stationCargo.quantity - rng.randNext(1, 4));
                        quantityUpdated = true;
                    }
                }
            }
        }

        updateCargoDistribution();

        auto w = WindowManager::find(WindowType::station, enumValue(id()));
        if (w != nullptr && (w->currentTab == 2 || w->currentTab == 1 || quantityUpdated))
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

        if ((flags & (StationFlags::flag_7 | StationFlags::flag_8)) == StationFlags::none && !CompanyManager::isPlayerCompany(owner))
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

        return std::clamp<int32_t>(rating, kMinCargoRating, kMaxCargoRating);
    }

    // 0x004929DB
    void Station::updateCargoDistribution()
    {
        invalidateWindow();
        WindowManager::invalidate(Ui::WindowType::stationList);
        bool hasChanged = false;
        for (uint8_t i = 0; i < kMaxCargoStats; ++i)
        {
            auto& stationCargo = cargoStats[i];
            auto newDensity = 0;
            if (stationCargo.quantity != 0)
            {
                if (stationTileSize != 0)
                {
                    newDensity = stationCargo.quantity / stationTileSize;
                    auto* cargoObj = ObjectManager::get<CargoObject>(i);
                    newDensity += (1 << cargoObj->var_14) - 1;
                    newDensity >>= cargoObj->var_14;

                    newDensity = std::min(newDensity, 15);
                }
            }
            if (stationCargo.densityPerTile != newDensity)
            {
                stationCargo.densityPerTile = newDensity;
                hasChanged = true;
            }
        }

        if (hasChanged)
        {
            for (auto i = 0; i < stationTileSize; ++i)
            {
                const auto& tile = stationTiles[i];
                Ui::ViewportManager::invalidate({ tile.x, tile.y }, World::heightFloor(tile.z), World::heightFloor(tile.z) + 32, ZoomLevel::full);
            }
        }
    }

    // 0x0048DCA5
    void Station::updateLabel()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x0048DCA5, regs);
    }

    // 0x004CBA2D
    void Station::invalidate()
    {
        Ui::ViewportManager::invalidate(this);
    }

    void Station::invalidateWindow()
    {
        WindowManager::invalidate(WindowType::station, enumValue(id()));
    }

    // 0x0048F6D4
    StationElement* getStationElement(const Pos3& pos)
    {
        auto tile = TileManager::get(pos.x, pos.y);
        auto baseZ = pos.z / 4;

        for (auto& element : tile)
        {
            auto* stationElement = element.as<StationElement>();

            if (stationElement == nullptr)
            {
                continue;
            }

            if (stationElement->baseZ() != baseZ)
            {
                continue;
            }

            if (!stationElement->isAiAllocated())
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
    static void setStationCatchmentRegion(CargoSearchState& cargoSearchState, TilePos2 minPos, TilePos2 maxPos, const CatchmentFlags flag)
    {
        minPos.x = std::max(minPos.x, static_cast<coord_t>(0));
        minPos.y = std::max(minPos.y, static_cast<coord_t>(0));
        maxPos.x = std::min(maxPos.x, static_cast<coord_t>(kMapColumns - 1));
        maxPos.y = std::min(maxPos.y, static_cast<coord_t>(kMapRows - 1));

        maxPos.x -= minPos.x;
        maxPos.y -= minPos.y;
        maxPos.x++;
        maxPos.y++;

        cargoSearchState.setTileRegion(minPos.x, minPos.y, maxPos.x, maxPos.y, flag);
    }

    // 0x00491BF5
    void sub_491BF5(const Pos2& pos, const CatchmentFlags flag)
    {
        auto minPos = World::toTileSpace(pos);
        auto maxPos = minPos;
        maxPos.x += catchmentSize;
        maxPos.y += catchmentSize;
        minPos.x -= catchmentSize;
        minPos.y -= catchmentSize;

        CargoSearchState cargoSearchState;

        setStationCatchmentRegion(cargoSearchState, minPos, maxPos, flag);
    }

    // 0x0048F716
    void recalculateStationCenter(const StationId stationId)
    {
        auto* station = StationManager::get(stationId);
        int32_t totalX = 0;
        int32_t totalY = 0;
        int16_t maxZ = 0;
        uint16_t count = 0;
        for (auto i = 0U; i < station->stationTileSize; ++i)
        {
            auto& tile = station->stationTiles[i];
            auto* elStation = getStationElement(tile);
            if (elStation == nullptr)
            {
                continue;
            }
            totalX += tile.x;
            totalY += tile.y;
            maxZ = std::max(World::heightFloor(tile.z), maxZ);
            count++;
        }
        if (count != 0)
        {
            station->z = maxZ;
            station->x = (totalX / count) + 16;
            station->y = (totalY / count) + 16;
            station->updateLabel();
        }
    }

    // 0x0048F529
    void recalculateStationModes(const StationId stationId)
    {
        auto* station = StationManager::get(stationId);
        uint32_t acceptedCargoTypes = 0;
        station->flags &= ~StationFlags::allModes;

        // This loop is similar to the start of doCalcAcceptedCargo except for the
        // following difference:
        // 1. It doesn't accept ghost station tiles
        // 2. It also works out the StationFlags transport mode
        // 3. RoadStations with both RoadStationFlags::passenger and freight unset
        //    are handled differently. (Perhaps vanilla bug)
        for (auto i = 0U; i < station->stationTileSize; ++i)
        {
            auto& pos = station->stationTiles[i];
            StationElement* elStation = getStationElement(pos);
            if (elStation == nullptr || elStation->isGhost())
            {
                continue;
            }
            switch (elStation->stationType())
            {
                case StationType::trainStation:
                {
                    auto* elTrack = elStation->prev()->as<TrackElement>();
                    if (elTrack == nullptr)
                    {
                        break;
                    }
                    auto* trackObj = ObjectManager::get<TrackObject>(elTrack->trackObjectId());
                    station->flags |= trackObj->hasFlags(TrackObjectFlags::unk_02)
                        ? StationFlags::transportModeRoad
                        : StationFlags::transportModeRail;
                    acceptedCargoTypes = ~0U;
                    break;
                }
                case StationType::roadStation:
                {
                    // Loop from start as road element can be variable number of elements below station
                    auto tile = TileManager::get(pos);
                    for (auto& el2 : tile)
                    {
                        if (&el2 == reinterpret_cast<TileElement*>(&elStation))
                        {
                            break;
                        }
                        auto* elRoad = el2.as<RoadElement>();
                        if (elRoad == nullptr)
                        {
                            continue;
                        }
                        if (elRoad->baseZ() != elStation->baseZ())
                        {
                            continue;
                        }
                        if (elRoad->isAiAllocated() || elRoad->isGhost())
                        {
                            continue;
                        }
                        auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                        station->flags |= roadObj->hasFlags(RoadObjectFlags::unk_01)
                            ? StationFlags::transportModeRail
                            : StationFlags::transportModeRoad;
                    }
                    auto* roadStationObj = ObjectManager::get<RoadStationObject>(elStation->objectId());
                    if (roadStationObj->hasFlags(RoadStationFlags::passenger))
                    {
                        acceptedCargoTypes |= 1U << roadStationObj->cargoType;
                    }
                    else if (roadStationObj->hasFlags(RoadStationFlags::freight))
                    {
                        // Exclude passengers from this station type
                        acceptedCargoTypes |= ~(1U << roadStationObj->cargoType);
                    }
                    else
                    {
                        acceptedCargoTypes = ~0U;
                    }
                    break;
                }
                case StationType::airport:
                    station->flags |= StationFlags::transportModeAir;
                    acceptedCargoTypes = ~0U;
                    break;
                case StationType::docks:
                    station->flags |= StationFlags::transportModeWater;
                    acceptedCargoTypes = ~0U;
                    break;
            }
        }

        for (auto cargoType = 0U; cargoType < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoType)
        {
            auto* cargoObj = ObjectManager::get<CargoObject>(cargoType);
            if (cargoObj == nullptr)
            {
                continue;
            }
            station->cargoStats[cargoType].flags &= ~StationCargoStatsFlags::acceptedFromProducer;
            if (acceptedCargoTypes & (1U << cargoType))
            {
                station->cargoStats[cargoType].flags |= StationCargoStatsFlags::acceptedFromProducer;
            }
        }
    }

    // 0x0048F321
    void addTileToStation(const StationId stationId, const World::Pos3& pos, uint8_t rotation)
    {
        auto* station = StationManager::get(stationId);
        station->stationTiles[station->stationTileSize] = pos;
        station->stationTiles[station->stationTileSize].z &= ~0x3;
        station->stationTiles[station->stationTileSize].z |= (rotation & 0x3);
        station->stationTileSize++;

        CargoSearchState cargoSearchState;
        const auto acceptedCargos = station->calcAcceptedCargo(cargoSearchState);

        if (cargoSearchState.cargoFilterHasBeenRecalculated() && (station->flags & StationFlags::flag_5) != StationFlags::none)
        {
            station->flags &= ~StationFlags::flag_5;
            auto* town = TownManager::get(station->town);
            town->numStations++;
            Ui::WindowManager::invalidate(WindowType::town, enumValue(station->town));

            for (auto& station2 : StationManager::stations())
            {
                if (station2.owner == station->owner)
                {
                    continue;
                }
                // THIS LOOKS WRONG WHY ISN'T IT station2.flags (vanilla mistake?)
                if ((station->flags & StationFlags::flag_5) != StationFlags::none)
                {
                    continue;
                }
                const auto distance = Math::Vector::manhattanDistance2D(World::Pos2{ station->x, station->y }, World::Pos2{ station2.x, station2.y });
                if (distance <= 256)
                {
                    companyEmotionEvent(station2.owner, Emotion::surprised);
                }
            }
        }
        for (auto cargoId = 0U; cargoId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoId)
        {
            auto& stats = station->cargoStats[cargoId];
            stats.industryId = cargoSearchState.getIndustry(cargoId);
            bool isAccepted = (acceptedCargos & (1 << cargoId)) != 0;
            stats.isAccepted(isAccepted);
        }
    }

    // 0x00491C6F
    void sub_491C6F(const uint8_t type, const Pos2& pos, const uint8_t rotation, const CatchmentFlags flag)
    {
        auto airportObject = ObjectManager::get<AirportObject>(type);

        auto [minPos, maxPos] = airportObject->getAirportExtents(World::toTileSpace(pos), rotation);

        minPos.x -= catchmentSize;
        minPos.y -= catchmentSize;
        maxPos.x += catchmentSize;
        maxPos.y += catchmentSize;

        CargoSearchState cargoSearchState;

        setStationCatchmentRegion(cargoSearchState, minPos, maxPos, flag);
    }

    // 0x00491D20
    void sub_491D20(const Pos2& pos, const CatchmentFlags flag)
    {
        auto minPos = World::toTileSpace(pos);
        auto maxPos = minPos + TilePos2{ 1, 1 };
        maxPos.x += catchmentSize;
        maxPos.y += catchmentSize;
        minPos.x -= catchmentSize;
        minPos.y -= catchmentSize;

        CargoSearchState cargoSearchState;

        setStationCatchmentRegion(cargoSearchState, minPos, maxPos, flag);
    }

    StringId getTransportIconsFromStationFlags(const StationFlags flags)
    {
        constexpr StringId label_icons[] = {
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

        const auto label = enumValue(flags & StationFlags::allModes);
        return label_icons[label];
    }
}
